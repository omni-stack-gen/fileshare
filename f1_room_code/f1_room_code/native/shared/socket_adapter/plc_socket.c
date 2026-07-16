#include "transport.h"
#include "transport_internal_log.h"
#include "transport_stat.h"

#include <goblin_transmission.h>
#include <utils/bmem.h>

#include <sys/eventfd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <inttypes.h>

#include <utils/list.h>

// #define DEBUG_STATISTICS
#define MAX_PLC_SOCKETS 32
#define DEFAULT_PLC_RX_CACHE_SIZE (512 * 1024)
#define MAX_PLC_DATA_SIZE	(2024)
// #define PLC_DEBUG_PORT 0x01U

// 用户态接收队列节点
typedef struct plc_rx_node
{
	struct list_head list;
	uint32_t addr;
	uint16_t port;
	uint32_t data_len;
	uint8_t  data[]; // 柔性数组，存放实际的音视频 payload
} plc_rx_node_t;

// Socket 控制块
typedef struct plc_sock_ctx
{
	int efd;              // eventfd，暴露给上层的 fd
	uint32_t listen_dev;  // 过滤目标 dev
	uint16_t listen_port; // 过滤目标 port
	bool is_closing;
	uint64_t open_time_ms;

	size_t max_rx_cache_size;

	// RX 相关
	struct list_head rx_queue;
	pthread_mutex_t  rx_lock;
	size_t rx_mem_used;
	size_t rx_mem_peak;
	uint32_t rx_pkt_count;
	uint32_t rx_pkt_peak;
	transport_internal_stat_t rx_stat;

	// TX 相关
	pthread_mutex_t tx_lock; // 发送专用的锁，不阻塞接收
	transport_internal_stat_t tx_stat;
} plc_sock_ctx_t;

static plc_sock_ctx_t *g_sock_table[MAX_PLC_SOCKETS] = {NULL};
static pthread_mutex_t g_sock_mutex = PTHREAD_MUTEX_INITIALIZER;

static uint64_t plc_time_now_ms(void)
{
	struct timespec now;

	if (clock_gettime(CLOCK_BOOTTIME, &now) != 0)
	{
		return 0;
	}

	return (uint64_t)now.tv_sec * 1000ULL + (uint64_t)now.tv_nsec / 1000000ULL;
}

static plc_sock_ctx_t *find_ctx_by_fd(int fd)
{
	if (fd < 0)
	{
		return NULL;
	}

	for (int i = 0; i < MAX_PLC_SOCKETS; i++)
	{
		if (g_sock_table[i] != NULL && g_sock_table[i]->efd == fd)
		{
			return g_sock_table[i];
		}
	}

	return NULL;
}

#ifdef PLC_DEBUG_PORT
static int plc_debug_port_enabled(uint16_t port)
{
	return port == PLC_DEBUG_PORT;
}
#endif


// 底层 SPI 收到数据时的回调函数 (生产者)
static void internal_socket_callback(void *param, uint32_t addr, uint8_t *data, uint16_t data_size)
{
	plc_sock_ctx_t *ctx = (plc_sock_ctx_t *)param;

	if (ctx == NULL || ctx->is_closing)
	{
		return;
	}

	uint64_t now = plc_time_now_ms();
	size_t node_size = sizeof(plc_rx_node_t) + data_size;

	pthread_mutex_lock(&ctx->rx_lock);

	transport_update_internal_stat(&ctx->rx_stat, data_size, now);

	if (ctx->rx_mem_used + node_size > ctx->max_rx_cache_size)
	{
		pthread_mutex_unlock(&ctx->rx_lock);
		TRANSPORT_LOGW("App Rx cache full. Drop packet from dev [0x%x]. size: [%zu/%zu] bytes\n", addr,
		               ctx->rx_mem_used + node_size, ctx->max_rx_cache_size);
		return;
	}

	plc_rx_node_t *node = (plc_rx_node_t *)bmalloc(node_size);

	if (node)
	{
		node->addr = addr;
		node->port = ctx->listen_port;
		node->data_len = data_size;

		if (data_size > 0 && data)
		{
			memcpy(node->data, data, data_size);
		}

#ifdef PLC_DEBUG_PORT
		if (plc_debug_port_enabled(ctx->listen_port))
		{
			TRANSPORT_LOGI("plc callback queued: listen_port=0x%02x src=0x%x len=%u\n",
			               ctx->listen_port, addr, data_size);
		}
#endif

		list_add_tail(&node->list, &ctx->rx_queue);
		ctx->rx_mem_used += node_size;
		ctx->rx_pkt_count++;

		if (ctx->rx_mem_used > ctx->rx_mem_peak)
		{
			ctx->rx_mem_peak = ctx->rx_mem_used;
		}

		if (ctx->rx_pkt_count > ctx->rx_pkt_peak)
		{
			ctx->rx_pkt_peak = ctx->rx_pkt_count;
		}
	}

	pthread_mutex_unlock(&ctx->rx_lock);

	uint64_t val = 1;

	if (write(ctx->efd, &val, sizeof(val)) < 0)
	{
		TRANSPORT_LOGE("eventfd write failed: %s\n", strerror(errno));
	}
}

static int plc_open(const char *interface, transport_endpoint_t *ep)
{
	if (ep == NULL)
	{
		return -1;
	}

	if (ep->port > 0xFF)
	{
		TRANSPORT_LOGE("Invalid open port %u, exceeds maximum limit 255\n", ep->port);
		return -1;
	}

	// 【优化 1：Fail-Fast 查表先行】
	pthread_mutex_lock(&g_sock_mutex);

	int assigned_idx = -1;

	for (int i = 0; i < MAX_PLC_SOCKETS; i++)
	{
		if (g_sock_table[i] == NULL)
		{
			assigned_idx = i;

			// 找到槽位后，直接在这里分配内存，并立刻占据槽位
			g_sock_table[i] = (plc_sock_ctx_t *)bmalloc(sizeof(plc_sock_ctx_t));

			if (g_sock_table[i] != NULL)
			{
				memset(g_sock_table[i], 0, sizeof(plc_sock_ctx_t));
			}

			break;
		}
	}

	pthread_mutex_unlock(&g_sock_mutex);

	if (assigned_idx < 0 || g_sock_table[assigned_idx] == NULL)
	{
		TRANSPORT_LOGE("Max socket limit (%d) reached or bmalloc failed!\n",
		               MAX_PLC_SOCKETS);
		return -1;
	}

	plc_sock_ctx_t *ctx = g_sock_table[assigned_idx];

	int efd = eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK | EFD_CLOEXEC);

	if (efd < 0)
	{
		// 回滚
		pthread_mutex_lock(&g_sock_mutex);
		g_sock_table[assigned_idx] = NULL;
		pthread_mutex_unlock(&g_sock_mutex);
		bfree(ctx);
		return -1;
	}

	ctx->efd = efd;
	ctx->listen_dev = ep->addr;
	ctx->listen_port = ep->port;
	ctx->max_rx_cache_size = DEFAULT_PLC_RX_CACHE_SIZE;
	ctx->open_time_ms = plc_time_now_ms();

	INIT_LIST_HEAD(&ctx->rx_queue);
	pthread_mutex_init(&ctx->rx_lock, NULL);
	pthread_mutex_init(&ctx->tx_lock, NULL);

	// 注册底层回调函数
	if (goblin_plc_register_recv_callback((uint8_t)ctx->listen_port, internal_socket_callback, ctx) != 0)
	{
		// 注册失败回滚
		pthread_mutex_lock(&g_sock_mutex);
		g_sock_table[assigned_idx] = NULL;
		pthread_mutex_unlock(&g_sock_mutex);

		close(efd);
		pthread_mutex_destroy(&ctx->rx_lock);
		pthread_mutex_destroy(&ctx->tx_lock);
		bfree(ctx);
		return -1;
	}

	TRANSPORT_LOGI("[SPI_PLC Open success] fd=%d, port=%u\n", efd, ep->port);
	return efd;
}

static int plc_send(int fd, transport_endpoint_t *ep, const void *data, uint32_t len)
{
	if (fd < 0)
	{
		TRANSPORT_LOGE("Invalid fd %d for send\n", fd);
		return -1;
	}

	if (ep->port > 0xFF)
	{
		TRANSPORT_LOGE("Invalid target port %u, exceeds limit\n", ep->port);
		return -1;
	}

	pthread_mutex_lock(&g_sock_mutex);

	if (find_ctx_by_fd(fd) == NULL)
	{
		pthread_mutex_unlock(&g_sock_mutex);
		TRANSPORT_LOGE("Socket fd %d not found or closed\n", fd);
		return -1;
	}

	pthread_mutex_unlock(&g_sock_mutex);

	int ret = goblin_plc_send_data(ep->addr, (uint8_t)ep->port, (uint8_t *)data, (uint16_t)len);

	if (ret != 0)
	{
		TRANSPORT_LOGE("Underlying send failed for dev [0x%x] port [0x%02x]\n", ep->addr, ep->port);
	}
	else
	{
		// 发送成功，更新 Tx 统计信息
		pthread_mutex_lock(&g_sock_mutex);

		plc_sock_ctx_t *ctx = find_ctx_by_fd(fd);

		if (ctx)
		{
			uint64_t now = plc_time_now_ms();
			pthread_mutex_lock(&ctx->tx_lock);
			transport_update_internal_stat(&ctx->tx_stat, len, now);
			pthread_mutex_unlock(&ctx->tx_lock);
		}

		pthread_mutex_unlock(&g_sock_mutex);
	}

	return ret;
}

static int plc_recv(int fd, transport_endpoint_t *ep, void *buf, uint32_t buf_len, int timeout_ms)
{
	if (fd < 0 || buf == NULL)
	{
		TRANSPORT_LOGE("Invalid arguments for recv\n");
		return -1;
	}

	pthread_mutex_lock(&g_sock_mutex);
	plc_sock_ctx_t *ctx = find_ctx_by_fd(fd);

	if (ctx == NULL)
	{
		pthread_mutex_unlock(&g_sock_mutex);
		TRANSPORT_LOGE("Socket fd %d not found or closed\n", fd);
		return -1;
	}

	pthread_mutex_unlock(&g_sock_mutex);

	// 1. 超时控制
	if (timeout_ms >= 0)
	{
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(fd, &read_fds);

		struct timeval tv;
		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;

		int ret = select(fd + 1, &read_fds, NULL, NULL, &tv);

		if (ret < 0)
		{
			if (errno == EINTR)
			{
				return -3;
			}

			TRANSPORT_LOGE("Select failed on fd %d: %s\n", fd, strerror(errno));
			return -1;
		}
		else if (ret == 0)
		{
			if (timeout_ms == 0)
			{
				return -3;    // EAGAIN
			}

			return -2; // Timeout
		}
	}

	// 2. 从 eventfd 中提取 1 个信号量
	uint64_t val;
	ssize_t ret = read(fd, &val, sizeof(val));

	if (ret < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			return -3; // 理论上 select 返回可读后不应触发，但非阻塞模式下需严谨处理
		}

		TRANSPORT_LOGE("eventfd read failed on fd %d: %s\n", fd, strerror(errno));
		return -1;
	}

	// 3. 从用户态缓冲队列中取出一个真实的包
	plc_rx_node_t *node = NULL;
	size_t node_size = 0;

	pthread_mutex_lock(&ctx->rx_lock);

	if (list_empty(&ctx->rx_queue))
	{
		pthread_mutex_unlock(&ctx->rx_lock);
		TRANSPORT_LOGE("Fatal logic error: eventfd signaled but user queue is empty!\n");
		return -1;
	}

	node = list_first_entry(&ctx->rx_queue, plc_rx_node_t, list);

	list_del(&node->list);

	node_size = sizeof(plc_rx_node_t) + node->data_len;

	// 更新计数，出队时只减小 current，peak 保持不变
	if (ctx->rx_mem_used >= node_size)
	{
		ctx->rx_mem_used -= node_size;
	}
	else
	{
		ctx->rx_mem_used = 0;
	}

	if (ctx->rx_pkt_count > 0)
	{
		ctx->rx_pkt_count--;
	}

	pthread_mutex_unlock(&ctx->rx_lock);

	// 4. 将数据拷贝给应用层并释放节点
	if (ep)
	{
		ep->addr = node->addr;
		ep->port = node->port;
	}

	int copy_len = (node->data_len > buf_len) ? buf_len : node->data_len;

	if (copy_len > 0)
	{
		memcpy(buf, node->data, copy_len);
	}

#ifdef PLC_DEBUG_PORT
	if (plc_debug_port_enabled(ctx->listen_port))
	{
		TRANSPORT_LOGI("plc recv ok: fd=%d listen_dev=0x%x listen_port=0x%02x src=0x%x:%u len=%d\n",
		               fd, ctx->listen_dev, ctx->listen_port,
		               ep ? ep->addr : 0, ep ? ep->port : 0, copy_len);
	}
#endif

	if (node->data_len > buf_len)
	{
		TRANSPORT_LOGW("User buffer too small (%u < %u), packet truncated!\n", buf_len, node->data_len);
	}

	bfree(node);

	return copy_len;
}

static void plc_close(int fd)
{
	if (fd < 0)
	{
		return;
	}

	pthread_mutex_lock(&g_sock_mutex);
	plc_sock_ctx_t *ctx = find_ctx_by_fd(fd);

	if (ctx == NULL)
	{
		pthread_mutex_unlock(&g_sock_mutex);
		return;
	}

	// 1. 设置 closing 标志，阻断 callback
	ctx->is_closing = true;

	// 2. 从全局表中摘除
	for (int i = 0; i < MAX_PLC_SOCKETS; i++)
	{
		if (g_sock_table[i] == ctx)
		{
			g_sock_table[i] = NULL;
			break;
		}
	}

	pthread_mutex_unlock(&g_sock_mutex);

	// 3 锁外注销回调，防止死锁
	goblin_plc_unregister_recv_callback((uint8_t)ctx->listen_port);

	TRANSPORT_LOGI("[SPI_PLC Socket Closed] fd: %d | dev: 0x%x | port: 0x%02x\n", fd, ctx->listen_dev, ctx->listen_port);

#ifdef DEBUG_STATISTICS
	// ==================== 统计打印 ====================
	transport_stat_t rx_info = {0}, tx_info = {0};

	uint64_t run_time = plc_time_now_ms() - ctx->open_time_ms;

	pthread_mutex_lock(&ctx->rx_lock);
	transport_fill_stat_info(&rx_info, &ctx->rx_stat, run_time, MAX_PLC_DATA_SIZE);
	pthread_mutex_unlock(&ctx->rx_lock);

	pthread_mutex_lock(&ctx->tx_lock);
	transport_fill_stat_info(&tx_info, &ctx->tx_stat, run_time, MAX_PLC_DATA_SIZE);
	pthread_mutex_unlock(&ctx->tx_lock);

		TRANSPORT_LOGI("=================================================================\n");
		TRANSPORT_LOGI("  Total Run Time     : %" PRIu64 " ms\n", run_time);
		TRANSPORT_LOGI("----------------------- MEM STATISTICS ---------------------------\n");
		TRANSPORT_LOGI("  Memory Peak        : %zu Bytes (%.2f MB), Limit: %zu MB\n", ctx->rx_mem_peak,
		               (float)ctx->rx_mem_peak / (1024 * 1024), ctx->max_rx_cache_size / (1024 * 1024));
		TRANSPORT_LOGI("  Packets Peak       : %u packets\n", ctx->rx_pkt_peak);
		TRANSPORT_LOGI("  Remain Before Drop : %zu Bytes, Packets: %u\n", ctx->rx_mem_used, ctx->rx_pkt_count);
		TRANSPORT_LOGI("----------------------- RX STATISTICS ---------------------------\n");
		TRANSPORT_LOGI("  Total Packets      : %" PRIu64 " pkts (Total Rx: %" PRIu64 " B)\n", rx_info.total_pkts, rx_info.total_bytes);
		TRANSPORT_LOGI("  Payload Efficiency : %.2f%% (Max: %d B/pkt)\n", rx_info.payload_utilization_pct,
		               (int)MAX_PLC_DATA_SIZE);
		TRANSPORT_LOGI("  Interval (ms)      : Avg: %" PRIu64 ", Min: %" PRIu64 ", Max: %" PRIu64 "\n", rx_info.avg_interval_ms,
		               rx_info.min_interval_ms, rx_info.max_interval_ms);
		TRANSPORT_LOGI("  Throughput (Kb/s)  : Global Avg: %.2f, Max 1s: %.2f, Min 1s: %.2f\n", (float)rx_info.avg_total_bps / 1024.0,
		               (float)rx_info.max_rate_bps / 1024.0, (float)rx_info.min_rate_bps / 1024.0);
		TRANSPORT_LOGI("  Packet Rate (pps)  : Global Avg: %u, Max 1s: %u, Min 1s: %u\n",  rx_info.avg_total_pps, rx_info.max_rate_pps,
		               rx_info.min_rate_pps);
		TRANSPORT_LOGI("----------------------- TX STATISTICS ---------------------------\n");
		TRANSPORT_LOGI("  Total Packets      : %" PRIu64 " pkts (Total Tx: %" PRIu64 " B)\n", tx_info.total_pkts, tx_info.total_bytes);
		TRANSPORT_LOGI("  Payload Efficiency : %.2f%% (Max: %d B/pkt)\n", tx_info.payload_utilization_pct,
		               (int)MAX_PLC_DATA_SIZE);
		TRANSPORT_LOGI("  Interval (ms)      : Avg: %" PRIu64 ", Min: %" PRIu64 ", Max: %" PRIu64 "\n", tx_info.avg_interval_ms,
		               tx_info.min_interval_ms, tx_info.max_interval_ms);
		TRANSPORT_LOGI("  Throughput (Kb/s)  : Global Avg: %.2f, Max 1s: %.2f, Min 1s: %.2f\n", (float)tx_info.avg_total_bps / 1024.0,
		               tx_info.max_rate_bps / 1024.0, tx_info.min_rate_bps / 1024.0);
		TRANSPORT_LOGI("  Packet Rate (pps)  : Global Avg: %u, Max 1s: %u, Min 1s: %u\n", tx_info.avg_total_pps, tx_info.max_rate_pps,
		               tx_info.min_rate_pps);
		TRANSPORT_LOGI("=================================================================\n");
#endif

	// 4. 清理残留队列
	pthread_mutex_lock(&ctx->rx_lock);
	plc_rx_node_t *node, *next;
	list_for_each_entry_safe(node, next, &ctx->rx_queue, list)
	{
		list_del(&node->list);
		bfree(node);
	}
	pthread_mutex_unlock(&ctx->rx_lock);

	// 4. 释放其余资源
	pthread_mutex_destroy(&ctx->rx_lock);
	pthread_mutex_destroy(&ctx->tx_lock);
	close(fd);

	bfree(ctx);
}

// ================= 新增查询接口 =================
static int plc_get_info(int fd, transport_info_t  *info)
{
	if (fd < 0 || info == NULL)
	{
		TRANSPORT_LOGE("Invalid arguments for get_info\n");
		return -1;
	}

	pthread_mutex_lock(&g_sock_mutex);

	plc_sock_ctx_t *ctx = find_ctx_by_fd(fd);

	if (ctx == NULL)
	{
		pthread_mutex_unlock(&g_sock_mutex);
		TRANSPORT_LOGE("Socket fd %d not found or closed\n", fd);
		return -1;
	}

	info->total_run_time_ms = plc_time_now_ms() - ctx->open_time_ms;

	pthread_mutex_lock(&ctx->rx_lock);
	info->current_mem = ctx->rx_mem_used;
	info->peak_mem = ctx->rx_mem_peak;
	info->current_rx_pkts = ctx->rx_pkt_count;
	info->peak_rx_pkts = ctx->rx_pkt_peak;
	transport_fill_stat_info(&info->rx, &ctx->rx_stat, info->total_run_time_ms, MAX_PLC_DATA_SIZE);
	pthread_mutex_unlock(&ctx->rx_lock);

	pthread_mutex_lock(&ctx->tx_lock);
	transport_fill_stat_info(&info->tx, &ctx->tx_stat, info->total_run_time_ms, MAX_PLC_DATA_SIZE);
	pthread_mutex_unlock(&ctx->tx_lock);

	pthread_mutex_unlock(&g_sock_mutex);

	return 0;
}

static int plc_setopt(int fd, transport_opt_t optname, const void *optval, uint32_t optlen)
{
	if (fd < 0 || optval == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sock_mutex);
	plc_sock_ctx_t *ctx = find_ctx_by_fd(fd);
	pthread_mutex_unlock(&g_sock_mutex);

	if (!ctx)
	{
		return -1;
	}

	if ((optname == TRANSPORT_OPT_MCAST_JOIN || optname == TRANSPORT_OPT_MCAST_LEAVE) &&
	    (optlen == sizeof(uint32_t) || optlen == sizeof(transport_mcast_opt_t)))
	{
		uint32_t mcast_addr = 0;
		uint8_t is_remove = (optname == TRANSPORT_OPT_MCAST_LEAVE) ? 1 : 0;

		if (optlen == sizeof(uint32_t))
		{
			/* Backward compatible mode */
			mcast_addr = *(const uint32_t *)optval;
		}
		else
		{
			const transport_mcast_opt_t *mcast_opt =
			    (const transport_mcast_opt_t *)optval;
			mcast_addr = mcast_opt->group_addr;
		}

		int ret = goblin_plc_multicast_ctrl(mcast_addr, is_remove);

		if (ret != 0)
		{
			TRANSPORT_LOGE("PLC fd %d multicast ctrl failed addr=0x%x isremove=%u\n",
			               fd, mcast_addr, is_remove);
			return -1;
		}

		TRANSPORT_LOGI("PLC fd %d multicast ctrl ok addr=0x%x isremove=%u\n",
		               fd, mcast_addr, is_remove);
		return 0;
	}

	// 响应 RCVBUF 选项，动态改变 PLC 用户态队列大小
	if (optname == TRANSPORT_OPT_RCVBUF && optlen == sizeof(int))
	{
		int val = *(int *)optval;

		if (val > 0)
		{
			pthread_mutex_lock(&ctx->rx_lock);
			ctx->max_rx_cache_size = (size_t)val;
			pthread_mutex_unlock(&ctx->rx_lock);

			TRANSPORT_LOGI("PLC fd %d set max_rx_cache_size to %d Bytes\n", fd, val);
			return 0;
		}
	}

	// 其他选项暂不支持
	return -1;
}

const transport_ops_t g_plc_transport_ops =
{
	.name = "SPI_PLC",
	.open = plc_open,
	.send = plc_send,
	.recv = plc_recv,
	.close = plc_close,
	.get_info = plc_get_info,
	.setopt = plc_setopt
};
