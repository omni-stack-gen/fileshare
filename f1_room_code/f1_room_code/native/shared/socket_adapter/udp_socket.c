#include "transport.h"
#include "transport_internal_log.h"
#include "transport_stat.h"

#include <utils/bmem.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#include <inttypes.h>

#define DEBUG_STATISTICS
#define MAX_UDP_SOCKETS 32
#define UDP_RECVBUF_TARGET_BYTES (512 * 1024)
// #define UDP_CTRL_DEBUG_PORT 10001U
// #define UDP_EXTRA_DEBUG_PORT 10000U

typedef struct udp_sock_ctx
{
	int sys_fd;
	uint32_t bind_addr;
	uint16_t bind_port;
	unsigned int bind_ifindex;
#ifdef UDP_CTRL_DEBUG_PORT
	int pktinfo_enabled;
#endif

	uint64_t open_time_ms;
	pthread_mutex_t rx_lock;
	transport_internal_stat_t rx_stat;

	pthread_mutex_t tx_lock;
	transport_internal_stat_t tx_stat;
} udp_sock_ctx_t;

static udp_sock_ctx_t *g_udp_table[MAX_UDP_SOCKETS] = {NULL};
static pthread_mutex_t g_udp_mutex = PTHREAD_MUTEX_INITIALIZER;

static uint64_t udp_time_now_ms(void)
{
	struct timespec now;

	if (clock_gettime(CLOCK_BOOTTIME, &now) != 0)
	{
		return 0;
	}

	return (uint64_t)now.tv_sec * 1000ULL + (uint64_t)now.tv_nsec / 1000000ULL;
}

static udp_sock_ctx_t *find_udp_ctx(int fd)
{
	if (fd < 0)
	{
		return NULL;
	}

	for (int i = 0; i < MAX_UDP_SOCKETS; i++)
	{
		if (g_udp_table[i] != NULL && g_udp_table[i]->sys_fd == fd)
		{
			return g_udp_table[i];
		}
	}

	return NULL;
}

static int udp_is_multicast_addr(uint32_t addr)
{
	return ((addr & 0xF0000000U) == 0xE0000000U) ? 1 : 0;
}

#ifdef UDP_CTRL_DEBUG_PORT
static int udp_debug_port_enabled(uint16_t port)
{
	if (port == UDP_CTRL_DEBUG_PORT)
	{
		return 1;
	}
#ifdef UDP_EXTRA_DEBUG_PORT
	if (port == UDP_EXTRA_DEBUG_PORT)
	{
		return 1;
	}
#endif
	return 0;
}
#endif

static void udp_configure_recv_buffer(int fd)
{
	int target = UDP_RECVBUF_TARGET_BYTES;
	int actual = 0;
	socklen_t optlen = sizeof(actual);

	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &target, sizeof(target)) != 0)
	{
		TRANSPORT_LOGW("setsockopt SO_RCVBUF failed, target=%d err=%s\n",
		               target, strerror(errno));
		return;
	}

	if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &actual, &optlen) != 0)
	{
		TRANSPORT_LOGW("getsockopt SO_RCVBUF failed err=%s\n", strerror(errno));
		return;
	}

	TRANSPORT_LOGI("udp recv buffer configured: target=%d actual=%d bytes\n",
	               target, actual);
}

static int udp_open(const char *interface, transport_endpoint_t *ep)
{
	if (ep == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_udp_mutex);

	int assigned_idx = -1;

	for (int i = 0; i < MAX_UDP_SOCKETS; i++)
	{
		if (g_udp_table[i] == NULL)
		{
			assigned_idx = i;
			g_udp_table[i] = (udp_sock_ctx_t *)bmalloc(sizeof(udp_sock_ctx_t));

			if (g_udp_table[i] != NULL)
			{
				memset(g_udp_table[i], 0, sizeof(udp_sock_ctx_t));
			}

			break;
		}
	}

	pthread_mutex_unlock(&g_udp_mutex);

	if (assigned_idx < 0 || g_udp_table[assigned_idx] == NULL)
	{
		TRANSPORT_LOGE("Max UDP socket limit (%d) reached or bmalloc failed!\n",
		               MAX_UDP_SOCKETS);
		return -1;
	}

	udp_sock_ctx_t *ctx = g_udp_table[assigned_idx];

	int fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0)
	{
		// 回滚
		pthread_mutex_lock(&g_udp_mutex);
		g_udp_table[assigned_idx] = NULL;
		pthread_mutex_unlock(&g_udp_mutex);
		bfree(ctx);
		return -1;
	}

	udp_configure_recv_buffer(fd);

#if defined(UDP_CTRL_DEBUG_PORT) && defined(IP_PKTINFO)
	{
		int pktinfo_on = 1;

		if (setsockopt(fd, IPPROTO_IP, IP_PKTINFO, &pktinfo_on, sizeof(pktinfo_on)) != 0)
		{
			TRANSPORT_LOGW("setsockopt IP_PKTINFO failed: %s\n", strerror(errno));
		}
		else
		{
			ctx->pktinfo_enabled = 1;
		}
	}
#endif

	if (interface && interface[0] != '\0')
	{
		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name));
		if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr)) != 0)
		{
			TRANSPORT_LOGW("setsockopt SO_BINDTODEVICE(%s) failed: %s\n",
			               interface, strerror(errno));
		}
		ctx->bind_ifindex = if_nametoindex(interface);

		if (ctx->bind_ifindex == 0)
		{
			TRANSPORT_LOGW("if_nametoindex(%s) failed: %s\n", interface, strerror(errno));
		}
	}

	// 地址复用
	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in saddr;
	const char *bind_ip = NULL;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(ep->port);
	saddr.sin_addr.s_addr = (ep->addr == ADDR_ANY) ? htonl(INADDR_ANY) : htonl(ep->addr);
	bind_ip = inet_ntoa(saddr.sin_addr);

	TRANSPORT_LOGI("udp bind try: if=%s addr=%s port=%u\n",
	               (interface && interface[0] != '\0') ? interface : "<any>",
	               bind_ip ? bind_ip : "<invalid>",
	               ep->port);

	if (bind(fd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
	{
		TRANSPORT_LOGE("UDP Bind failed: if=%s addr=%s port=%u err=%s\n",
		               (interface && interface[0] != '\0') ? interface : "<any>",
		               bind_ip ? bind_ip : "<invalid>",
		               ep->port, strerror(errno));

		// 绑定失败回滚
		pthread_mutex_lock(&g_udp_mutex);
		g_udp_table[assigned_idx] = NULL;
		pthread_mutex_unlock(&g_udp_mutex);
		close(fd);
		bfree(ctx);
		return -1;
	}

	// 初始化上下文
	ctx->sys_fd = fd;
	ctx->bind_addr = ep->addr;
	ctx->bind_port = ep->port;
	ctx->open_time_ms = udp_time_now_ms();
	pthread_mutex_init(&ctx->rx_lock, NULL);
	pthread_mutex_init(&ctx->tx_lock, NULL);

	TRANSPORT_LOGI("[STD_UDP Open success] fd=%d, if=%s addr=%s port=%u\n",
	               fd,
	               (interface && interface[0] != '\0') ? interface : "<any>",
	               bind_ip ? bind_ip : "<invalid>",
	               ep->port);
	return fd;
}

static int udp_send(int fd, transport_endpoint_t *ep, const void *data, uint32_t len)
{
	if (fd < 0 || !data)
	{
		return -1;
	}

	udp_sock_ctx_t *ctx = find_udp_ctx(fd);

	if (!ctx)
	{
		return -1;
	}

	struct sockaddr_in dest;
	memset(&dest, 0, sizeof(dest));

	dest.sin_family = AF_INET;
	dest.sin_port = htons(ep->port);
	dest.sin_addr.s_addr = htonl(ep->addr);

	ssize_t ret = sendto(fd, data, len, 0, (struct sockaddr *)&dest, sizeof(dest));

	if (ret >= 0)
	{
		// 发送成功，更新 Tx
		udp_sock_ctx_t *ctx = find_udp_ctx(fd);

		if (ctx)
		{
			uint64_t now = udp_time_now_ms();
			pthread_mutex_lock(&ctx->tx_lock);
			transport_update_internal_stat(&ctx->tx_stat, ret, now);
			pthread_mutex_unlock(&ctx->tx_lock);
#ifdef UDP_CTRL_DEBUG_PORT
			if (udp_debug_port_enabled(ctx->bind_port) ||
			    (ep && udp_debug_port_enabled(ep->port)))
			{
				TRANSPORT_LOGI("udp sendto ok: fd=%d local_port=%u dst=%s:%u len=%u\n",
				               fd, ctx->bind_port, inet_ntoa(dest.sin_addr),
				               ntohs(dest.sin_port), len);
			}
#endif
		}

		return 0;
	}

	TRANSPORT_LOGE("udp sendto failed: fd=%d dst=%s:%u len=%u err=%s\n",
	               fd, inet_ntoa(dest.sin_addr), ntohs(dest.sin_port), len, strerror(errno));

	return -1;
}

static int udp_recv(int fd, transport_endpoint_t *ep, void *buf, uint32_t buf_len, int timeout_ms)
{
	udp_sock_ctx_t *ctx = find_udp_ctx(fd);
#if defined(UDP_CTRL_DEBUG_PORT) && defined(IP_PKTINFO)
	char ctrl[CMSG_SPACE(sizeof(struct in_pktinfo))];
#endif

	if (!ctx)
	{
		return -1;
	}

	if (timeout_ms >= 0)
	{
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(fd, &read_fds);
		struct timeval tv = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
		int ret = select(fd + 1, &read_fds, NULL, NULL, &tv);

		if (ret < 0)
		{
			return (errno == EINTR) ? -3 : -1;
		}

		if (ret == 0)
		{
			return (timeout_ms == 0) ? -3 : -2;
		}
	}

	struct sockaddr_in src;
	ssize_t nbytes;
#ifdef UDP_CTRL_DEBUG_PORT
	unsigned int rx_ifindex = 0;
	char rx_ifname[IF_NAMESIZE] = {0};
#endif

#if defined(UDP_CTRL_DEBUG_PORT) && defined(IP_PKTINFO)
	struct msghdr msg;
	struct iovec iov;
	int recv_flags = (timeout_ms >= 0) ? MSG_DONTWAIT : 0;

	memset(&src, 0, sizeof(src));
	memset(&msg, 0, sizeof(msg));
	memset(&iov, 0, sizeof(iov));
	iov.iov_base = buf;
	iov.iov_len = buf_len;
	msg.msg_name = &src;
	msg.msg_namelen = sizeof(src);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	if (ctx->pktinfo_enabled)
	{
		memset(ctrl, 0, sizeof(ctrl));
		msg.msg_control = ctrl;
		msg.msg_controllen = sizeof(ctrl);
	}

	nbytes = recvmsg(fd, &msg, recv_flags);
#else
	socklen_t src_len = sizeof(src);
	int recv_flags = (timeout_ms >= 0) ? MSG_DONTWAIT : 0;

	memset(&src, 0, sizeof(src));
	nbytes = recvfrom(fd, buf, buf_len, recv_flags, (struct sockaddr *)&src, &src_len);
#endif

	if (nbytes <= 0)
	{
		if (nbytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
		{
			return -3;
		}
		return (nbytes == 0) ? 0 : -1;
	}

	if (ep)
	{
		ep->addr = ntohl(src.sin_addr.s_addr);
		ep->port = ntohs(src.sin_port);
	}

	if (nbytes > 0)
	{
		uint64_t now = udp_time_now_ms();
		pthread_mutex_lock(&ctx->rx_lock);
		transport_update_internal_stat(&ctx->rx_stat, nbytes, now);
		pthread_mutex_unlock(&ctx->rx_lock);

#ifdef UDP_CTRL_DEBUG_PORT
#ifdef IP_PKTINFO
		if (ctx->pktinfo_enabled)
		{
			struct cmsghdr *cmsg;

			for (cmsg = CMSG_FIRSTHDR(&msg);
			     cmsg != NULL;
			     cmsg = CMSG_NXTHDR(&msg, cmsg))
			{
				if (cmsg->cmsg_level == IPPROTO_IP &&
				    cmsg->cmsg_type == IP_PKTINFO &&
				    cmsg->cmsg_len >= CMSG_LEN(sizeof(struct in_pktinfo)))
				{
					const struct in_pktinfo *pktinfo =
						(const struct in_pktinfo *)CMSG_DATA(cmsg);

					rx_ifindex = pktinfo->ipi_ifindex;
					if (rx_ifindex > 0)
					{
						(void)if_indextoname(rx_ifindex, rx_ifname);
					}
					break;
				}
			}
		}
#endif
		if (udp_debug_port_enabled(ctx->bind_port) ||
		    (ep && udp_debug_port_enabled(ep->port)))
		{
			TRANSPORT_LOGI("udp recvfrom ok: fd=%d local_port=%u if=%s ifindex=%u src=%s:%u len=%d\n",
			               fd, ctx->bind_port,
			               rx_ifname[0] != '\0' ? rx_ifname : "<unknown>",
			               rx_ifindex,
			               inet_ntoa(src.sin_addr),
			               ntohs(src.sin_port), (int)nbytes);
		}
#endif
	}

	return (int)nbytes;
}

static void udp_close(int fd)
{
	if (fd < 0)
	{
		return;
	}

	pthread_mutex_lock(&g_udp_mutex);
	udp_sock_ctx_t *ctx = find_udp_ctx(fd);

	if (!ctx)
	{
		pthread_mutex_unlock(&g_udp_mutex);
		return;
	}

	// 【修改】将其从全局指针表中剔除
	for (int i = 0; i < MAX_UDP_SOCKETS; i++)
	{
		if (g_udp_table[i] == ctx)
		{
			g_udp_table[i] = NULL;
			break;
		}
	}

	pthread_mutex_unlock(&g_udp_mutex);

	TRANSPORT_LOGI("[STD_UDP Socket Closed] fd: %d\n", fd);

#ifdef DEBUG_STATISTICS
	// ==================== 统计打印 ====================
	uint64_t run_time = udp_time_now_ms() - ctx->open_time_ms;
	transport_stat_t rx_info = {0}, tx_info = {0};

	pthread_mutex_lock(&ctx->rx_lock);
	transport_fill_stat_info(&rx_info, &ctx->rx_stat, run_time, 0);
	pthread_mutex_unlock(&ctx->rx_lock);

	pthread_mutex_lock(&ctx->tx_lock);
	transport_fill_stat_info(&tx_info, &ctx->tx_stat, run_time, 0);
	pthread_mutex_unlock(&ctx->tx_lock);

	TRANSPORT_LOGI("=================================================================\n");
	TRANSPORT_LOGI("  Total Run Time     : %" PRIu64 " ms\n", run_time);
	TRANSPORT_LOGI("----------------------- RX STATISTICS ---------------------------\n");
	TRANSPORT_LOGI("  Total Packets      : %" PRIu64 " pkts (Total Rx: %" PRIu64 " B)\n", rx_info.total_pkts, rx_info.total_bytes);
	TRANSPORT_LOGI("  Interval (ms)      : Avg: %" PRIu64 ", Min: %" PRIu64 ", Max: %" PRIu64 "\n", rx_info.avg_interval_ms,
					rx_info.min_interval_ms, rx_info.max_interval_ms);
	TRANSPORT_LOGI("  Rate (Kb/s)        : Global Avg: %.2f, Max 1s: %.2f, Min 1s: %.2f\n", (float)rx_info.avg_total_bps / 1024.0,
					(float)rx_info.max_rate_bps / 1024.0, (float)rx_info.min_rate_bps / 1024.0);
	TRANSPORT_LOGI("----------------------- TX STATISTICS ---------------------------\n");
	TRANSPORT_LOGI("  Total Packets      : %" PRIu64 " pkts (Total Tx: %" PRIu64 " B)\n", tx_info.total_pkts, tx_info.total_bytes);
	TRANSPORT_LOGI("  Interval (ms)      : Avg: %" PRIu64 ", Min: %" PRIu64 ", Max: %" PRIu64 "\n", tx_info.avg_interval_ms,
					tx_info.min_interval_ms, tx_info.max_interval_ms);
	TRANSPORT_LOGI("  Rate (Kb/s)        : Global Avg: %.2f, Max 1s: %.2f, Min 1s: %.2f\n", (float)tx_info.avg_total_bps / 1024.0,
					tx_info.max_rate_bps / 1024.0, tx_info.min_rate_bps / 1024.0);
	TRANSPORT_LOGI("=================================================================\n");
#endif

	pthread_mutex_destroy(&ctx->rx_lock);
	pthread_mutex_destroy(&ctx->tx_lock);
	close(fd);

	bfree(ctx);
}

static int udp_get_info(int fd, transport_info_t *info)
{
	if (fd < 0 || info == NULL)
	{
		TRANSPORT_LOGE("Invalid arguments for get_info\n");
		return -1;
	}

	pthread_mutex_lock(&g_udp_mutex);

	udp_sock_ctx_t *ctx = find_udp_ctx(fd);

	if (ctx == NULL)
	{
		TRANSPORT_LOGE("Socket fd %d not found or closed\n", fd);
		return -1;
	}

	info->total_run_time_ms = udp_time_now_ms() - ctx->open_time_ms;

	pthread_mutex_lock(&ctx->rx_lock);
	// UDP 特性：内核缓冲，无法统计应用态内存，全置 0
	info->current_mem = info->peak_mem = 0;
	info->current_rx_pkts = info->peak_rx_pkts = 0;
	transport_fill_stat_info(&info->rx, &ctx->rx_stat, info->total_run_time_ms, 0);
	pthread_mutex_unlock(&ctx->rx_lock);

	pthread_mutex_lock(&ctx->tx_lock);
	transport_fill_stat_info(&info->tx, &ctx->tx_stat, info->total_run_time_ms, 0);
	pthread_mutex_unlock(&ctx->tx_lock);

	pthread_mutex_unlock(&g_udp_mutex);
	return 0;
}

static int udp_setopt(int fd, transport_opt_t optname, const void *optval, uint32_t optlen)
{
	int sys_opt = 0;

	if (optname == TRANSPORT_OPT_MCAST_JOIN || optname == TRANSPORT_OPT_MCAST_LEAVE)
	{
		if (!optval)
		{
			return -1;
		}

		transport_mcast_opt_t mcast_opt;
		memset(&mcast_opt, 0, sizeof(mcast_opt));

		if (optlen == sizeof(uint32_t))
		{
			/* Backward compatible mode */
			mcast_opt.group_addr = *(const uint32_t *)optval;
		}
		else if (optlen == sizeof(transport_mcast_opt_t))
		{
			mcast_opt = *(const transport_mcast_opt_t *)optval;
		}
		else
		{
			return -1;
		}

		if (!udp_is_multicast_addr(mcast_opt.group_addr))
		{
			TRANSPORT_LOGE("invalid multicast addr: 0x%x\n", mcast_opt.group_addr);
			return -1;
		}

		pthread_mutex_lock(&g_udp_mutex);
		udp_sock_ctx_t *ctx = find_udp_ctx(fd);
		pthread_mutex_unlock(&g_udp_mutex);

		if (!ctx)
		{
			return -1;
		}

		struct ip_mreqn mreqn;
		memset(&mreqn, 0, sizeof(mreqn));
		mreqn.imr_multiaddr.s_addr = htonl(mcast_opt.group_addr);

		int ifindex = mcast_opt.ifindex;
		uint32_t ifaddr = mcast_opt.if_addr;

		/*
		 * Prefer explicit caller config from transport_mcast_opt_t.
		 * If absent, fall back to socket bind hints for compatibility.
		 */
		if (ifindex <= 0 && ctx->bind_ifindex > 0)
		{
			ifindex = (int)ctx->bind_ifindex;
		}

		if (ifaddr == 0 && ctx->bind_addr != 0 && ctx->bind_addr != ADDR_ANY)
		{
			ifaddr = ctx->bind_addr;
		}

		mreqn.imr_ifindex = (ifindex > 0) ? ifindex : 0;
		mreqn.imr_address.s_addr = (ifaddr != 0) ? htonl(ifaddr) : htonl(INADDR_ANY);

		sys_opt = (optname == TRANSPORT_OPT_MCAST_JOIN) ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;
		return setsockopt(fd, IPPROTO_IP, sys_opt, &mreqn, sizeof(mreqn));
	}

	switch (optname)
	{
		case TRANSPORT_OPT_RCVBUF:
			sys_opt = SO_RCVBUF;
			break;

		case TRANSPORT_OPT_SNDBUF:
			sys_opt = SO_SNDBUF;
			break;

		case TRANSPORT_OPT_REUSEADDR:
			sys_opt = SO_REUSEADDR;
			break;

		case TRANSPORT_OPT_BINDTODEVICE:
			sys_opt = SO_BINDTODEVICE;
			break;

		case TRANSPORT_OPT_MCAST_LOOP:
		{
			unsigned char loop = 0;

			if (!optval || optlen < sizeof(int))
			{
				return -1;
			}

			loop = (*(const int *)optval) ? 1 : 0;
			return setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
		}

		case TRANSPORT_OPT_NONBLOCK:
		{
			int flags;
			bool enable;

			if (!optval || optlen < sizeof(int))
			{
				return -1;
			}

			flags = fcntl(fd, F_GETFL, 0);
			if (flags < 0)
			{
				return -1;
			}

			enable = (*(const int *)optval) != 0;
			if (enable)
			{
				flags |= O_NONBLOCK;
			}
			else
			{
				flags &= ~O_NONBLOCK;
			}
			return fcntl(fd, F_SETFL, flags);
		}

		default:
			return -1;
	}

	return setsockopt(fd, SOL_SOCKET, sys_opt, optval, optlen);
}

const transport_ops_t g_udp_transport_ops =
{
	.name = "STD_UDP",
	.open = udp_open,
	.send = udp_send,
	.recv = udp_recv,
	.close = udp_close,
	.get_info = udp_get_info,
	.setopt = udp_setopt
};
