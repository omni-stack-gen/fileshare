#include <utils/bmem.h>
#include <utils/rb_tree.h>
#include <utils/atomic.h>
#include <pthread.h>

#include <inttypes.h>

#define LOG_TAG "bmem"
#include <utils/log.h>

/////////////////////////

#if HEAP_TRACE

#define HEAP_MEM_DBG_ON 1
#define HEAP_MEM_ERR_ON 1

#define HEAP_MEM_DBG_MIN_SIZE 100
#define HEAP_MEM_MAX_CNT 4096
// #define HEAP_SYSLOG printf
#define HEAP_SYSLOG LOGI

// #define HEAP_MEM_IS_TRACED(size) (size > HEAP_MEM_DBG_MIN_SIZE)
#define HEAP_MEM_IS_TRACED(size) (0)

#define HEAP_MEM_LOG(flags, fmt, arg...) \
	do                                   \
	{                                    \
		if (flags)                       \
			HEAP_SYSLOG(fmt, ##arg);     \
	} while (0)

#define HEAP_MEM_DBG(fmt, arg...) \
	HEAP_MEM_LOG(HEAP_MEM_DBG_ON, "[heap] " fmt, ##arg)

#define HEAP_MEM_ERR(fmt, arg...)                             \
	HEAP_MEM_LOG(HEAP_MEM_ERR_ON, "[heap ERR] %s():%d, " fmt, \
	             __func__, __LINE__, ##arg);

void *__real_malloc(size_t size);
void *__real_realloc(void *ptr, size_t size);
void *__real_calloc(size_t num, size_t size);
void __real_free(void *ptr);

struct heap_mem
{
	void *ptr;
	int size;
};

static struct heap_mem g_mem[HEAP_MEM_MAX_CNT];

static int g_mem_entry_cnt = 0;
static int g_mem_entry_cnt_max = 0;
static int g_mem_empty_idx = 0; /* beginning idx to do the search for new one */

static int g_mem_sum = 0;
static int g_mem_sum_max = 0;
static int g_mem_max = 0;

static uint8_t g_do_reallocing = 0; /* flag for realloc() */

static pthread_mutex_t g_malloc_mutex = PTHREAD_MUTEX_INITIALIZER;

// #define WRAP_MEM_MAGIC_LEN 4
#define WRAP_MEM_MAGIC_LEN 0

#if (WRAP_MEM_MAGIC_LEN)
static const char g_mem_magic[WRAP_MEM_MAGIC_LEN] = {0x4a, 0x5b, 0x6c, 0x7f};
#define WRAP_MEM_SET_MAGIC(p, l) memcpy((((char *)(p)) + (l)), g_mem_magic, 4)
#define WRAP_MEM_CHK_MAGIC(p, l) memcmp((((char *)(p)) + (l)), g_mem_magic, 4)
#else
#define WRAP_MEM_SET_MAGIC(p, l) \
	do                           \
	{                            \
	} while (0)
#define WRAP_MEM_CHK_MAGIC(p, l) 0
#endif

#define malloc_mutex_lock() pthread_mutex_lock(&g_malloc_mutex)

#define malloc_mutex_unlock() pthread_mutex_unlock(&g_malloc_mutex)

/* Note: @ptr != NULL */
static void wrap_malloc_add_entry(void *ptr, size_t size)
{
	int i;

	WRAP_MEM_SET_MAGIC(ptr, size);

	for (i = g_mem_empty_idx; i < HEAP_MEM_MAX_CNT; ++i)
	{
		if (g_mem[i].ptr == NULL)
		{
			g_mem[i].ptr = ptr;
			g_mem[i].size = size;
			g_mem_entry_cnt++;
			g_mem_empty_idx = i + 1;
			g_mem_sum += size;

			if (g_mem_sum > g_mem_sum_max)
			{
				g_mem_sum_max = g_mem_sum;
			}

			if (size > g_mem_max)
			{
				g_mem_max = size;
			}

			if (g_mem_entry_cnt > g_mem_entry_cnt_max)
			{
				g_mem_entry_cnt_max = g_mem_entry_cnt;
			}

			break;
		}
	}

	if (i >= HEAP_MEM_MAX_CNT)
	{
		HEAP_MEM_ERR("heap mem count exceed %d\n", HEAP_MEM_MAX_CNT);
	}
}

/* Note: @ptr != NULL */
static ssize_t wrap_malloc_delete_entry(void *ptr)
{
	int i;
	ssize_t size;

	for (i = 0; i < HEAP_MEM_MAX_CNT; ++i)
	{
		if (g_mem[i].ptr == ptr)
		{
			size = g_mem[i].size;

			if (WRAP_MEM_CHK_MAGIC(ptr, size))
			{
				HEAP_MEM_ERR("mem f (%p, %d) corrupt\n", ptr, (int)size);
			}

			g_mem_sum -= size;
			g_mem[i].ptr = NULL;
			g_mem[i].size = 0;
			g_mem_entry_cnt--;

			if (i < g_mem_empty_idx)
			{
				g_mem_empty_idx = i;
			}

			break;
		}
	}

	if (i >= HEAP_MEM_MAX_CNT)
	{
		HEAP_MEM_ERR("heap mem entry (%p) missed entry_cnt:%d\n", ptr, g_mem_entry_cnt);
		size = -1;
	}

	return size;
}

/* Note: @old_ptr != NULL, @new_ptr != NULL, @new_size != 0 */
static ssize_t wrap_malloc_update_entry(void *old_ptr,
                                        void *new_ptr, size_t new_size)
{
	int i;
	ssize_t old_size;

	WRAP_MEM_SET_MAGIC(new_ptr, new_size);

	for (i = 0; i < HEAP_MEM_MAX_CNT; ++i)
	{
		if (g_mem[i].ptr == old_ptr)
		{
			old_size = g_mem[i].size;
			g_mem_sum = g_mem_sum - old_size + new_size;
			g_mem[i].ptr = new_ptr;
			g_mem[i].size = new_size;

			if (g_mem_sum > g_mem_sum_max)
			{
				g_mem_sum_max = g_mem_sum;
			}

			break;
		}
	}

	if (i >= HEAP_MEM_MAX_CNT)
	{
		HEAP_MEM_ERR("heap mem entry (%p) missed\n", new_ptr);
		old_size = -1;
	}

	return old_size;
}

void *__wrap_malloc(size_t size)
{
	malloc_mutex_lock();

	size_t real_size = size;

	if (!g_do_reallocing)
	{
		real_size += WRAP_MEM_MAGIC_LEN;
	}

	void *ptr = __real_malloc(real_size);

	if (!g_do_reallocing)
	{
		if (HEAP_MEM_IS_TRACED(size))
		{
			HEAP_MEM_DBG("malloc (%p, %d)\n", ptr, (int)size);
		}

		if (ptr)
		{
			wrap_malloc_add_entry(ptr, size);
		}
		else
		{
			HEAP_MEM_ERR("heap mem exhausted (%d)\n", (int)size);
		}
	}

	malloc_mutex_unlock();

	return ptr;
}

void *__wrap_calloc(size_t num, size_t size)
{
	malloc_mutex_lock();

	size_t real_size = size * num;

	if (!g_do_reallocing)
	{
		real_size += WRAP_MEM_MAGIC_LEN;
	}

	void *ptr = __real_calloc(num, size);

	if (!g_do_reallocing)
	{
		if (HEAP_MEM_IS_TRACED(real_size))
		{
			HEAP_MEM_DBG("m (%p, %u)\n", ptr, real_size);
		}

		if (ptr)
		{
			wrap_malloc_add_entry(ptr, real_size);
		}
		else
		{
			HEAP_MEM_ERR("heap mem exhausted (%u)\n", real_size);
		}
	}

	malloc_mutex_unlock();

	return ptr;
}

void *__wrap_realloc(void *ptr, size_t size)
{
	void *new_ptr;
	ssize_t old_size;

	malloc_mutex_lock();
	g_do_reallocing = 1;

	size_t real_size = size;

	if (size != 0)
	{
		/* (size == 0) means free it */
		real_size += WRAP_MEM_MAGIC_LEN;
	}

	new_ptr = __real_realloc(ptr, real_size);

	if (ptr == NULL)
	{
		old_size = 0;

		if (new_ptr != NULL)
		{
			wrap_malloc_add_entry(new_ptr, size);
		}
		else
		{
			if (size != 0)
			{
				HEAP_MEM_ERR("heap mem exhausted (%p, %d)\n", ptr, (int)size);
				goto out;
			}
		}
	}
	else
	{
		if (size == 0)
		{
			if (new_ptr != NULL)
			{
				HEAP_MEM_ERR("realloc (%p, %d) return %p\n", ptr, (int)size, new_ptr);
			}

			old_size = wrap_malloc_delete_entry(ptr);
		}
		else
		{
			if (new_ptr != NULL)
			{
				old_size = wrap_malloc_update_entry(ptr, new_ptr, (int)size);
			}
			else
			{
				HEAP_MEM_ERR("heap mem exhausted (%p, %d)\n", ptr, (int)size);
				goto out;
			}
		}
	}

	if (HEAP_MEM_IS_TRACED(size) || HEAP_MEM_IS_TRACED(old_size))
	{
		HEAP_MEM_DBG("realloc (%p, %d) <- (%p, %d)\n", new_ptr, (int)size, ptr, (int)old_size);
	}

out:
	g_do_reallocing = 0;
	malloc_mutex_unlock();

	return new_ptr;
}

void __wrap_free(void *ptr)
{
	malloc_mutex_lock();

	if (!g_do_reallocing && ptr)
	{
		ssize_t size = wrap_malloc_delete_entry(ptr);

		if (HEAP_MEM_IS_TRACED(size))
		{
			HEAP_MEM_DBG("free (%p, %d)\n", ptr, (int)size);
		}

		if (size != -1)
		{
			__real_free(ptr);
		}
	}

	malloc_mutex_unlock();
}

#endif

static void trace_heap_info(int verbose)
{
#if HEAP_TRACE
	malloc_mutex_lock();

#if 0
	HEAP_SYSLOG("\n<<< heap info >>>\n"
	            "g_mem_sum       %d (%d KB)\n"
	            "g_mem_sum_max   %d (%d KB)\n"
	            "g_mem_entry_cnt %d, max %d\n",
	            g_mem_sum, g_mem_sum / 1024,
	            g_mem_sum_max, g_mem_sum_max / 1024,
	            g_mem_entry_cnt, g_mem_entry_cnt_max);
#else
	HEAP_SYSLOG("<<< heap info >>>\n");
	HEAP_SYSLOG("g_mem_max       %d (%d KB)\n", g_mem_max, g_mem_max / 1024);
	HEAP_SYSLOG("g_mem_sum       %d (%d KB)\n", g_mem_sum, g_mem_sum / 1024);
	HEAP_SYSLOG("g_mem_sum_max   %d (%d KB)\n", g_mem_sum_max, g_mem_sum_max / 1024);
	HEAP_SYSLOG("g_mem_entry_cnt %d, max %d\n", g_mem_entry_cnt, g_mem_entry_cnt_max);
#endif

	int i, j = 0;

	for (i = 0; i < HEAP_MEM_MAX_CNT; ++i)
	{
		if (g_mem[i].ptr != NULL)
		{
			if (verbose)
			{
				HEAP_SYSLOG("%03d. %03d, %p, %d\n",
				            ++j, i, g_mem[i].ptr, g_mem[i].size);
			}

			if (WRAP_MEM_CHK_MAGIC(g_mem[i].ptr, g_mem[i].size))
			{
				HEAP_MEM_ERR("mem (%p) corrupt\n", g_mem[i].ptr);
			}
		}
	}

	malloc_mutex_unlock();
#endif
}

////////////////////////

#define ALIGNMENT 32
#define ALIGNMENT_HACK 1

#if DEBUG_MEMORY

#define SIG_SIZE            (sizeof(uint64_t))
#define SIG_BEGIN           (0x600D600D600D600D)
#define SIG_END             (0x0BAD0BAD0BAD0BAD)

typedef uint64_t sig_type;

typedef struct heap_item
{
	struct rb_node node;
	char *file;
	int line;
	void *ptr;
	size_t size;
} heap_item_t;

typedef struct heap_info
{
	size_t current_size;
	size_t max_size;
} heap_info_t;

static pthread_mutex_t heap_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct rb_root heap_tree = {.rb_node = NULL};

static heap_info_t heap_state = {0, 0};

static void apply_signature(void *p, size_t size)
{
	sig_type sig;

	sig = SIG_BEGIN;
	memcpy(p, &sig, SIG_SIZE);

	sig = SIG_END;
	memcpy(((char *)p) + SIG_SIZE + size, &sig, SIG_SIZE);
}

static void check_signature(const char *file, int line, void *p, size_t size)
{
	sig_type sig;

	uint8_t *mem = (uint8_t *)p;

	sig = SIG_BEGIN;

	if (0 != memcmp(mem - SIG_SIZE, &sig, SIG_SIZE))
	{
		LOGE("invalid start signature 0x%" PRIx64 "in heap item at file %s line %d\n",
		     *(sig_type *)(mem - SIG_SIZE), file, line);
	}

	sig = SIG_END;

	if (0 != memcmp(mem + size, &sig, SIG_SIZE))
	{
		LOGE("invalid end signature 0x%" PRIx64 "in heap item at file %s line %d\n",
		     *(sig_type *)(mem + size), file, line);
	}
}

static const char *get_file_name(const char *path)
{
	char *filename = strrchr(path, '/');

	return filename ? (filename + 1) : path;
}

static bool heap_item_add(struct rb_root *root, heap_item_t *item)
{
	struct rb_node **_new = &(root->rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*_new)
	{
		heap_item_t *_this = rb_entry(*_new, heap_item_t, node);

		parent = *_new;

		if (item->ptr < _this->ptr)
		{
			_new = &((*_new)->rb_left);
		}
		else if (item->ptr > _this->ptr)
		{
			_new = &((*_new)->rb_right);
		}
		else
		{
			LOGE("heap item add fail %p %p\n", item->ptr, _this->ptr);
			return false;
		}
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&item->node, parent, _new);
	rb_insert_color(&item->node, root);

	return true;
}

static heap_item_t *heap_item_find(struct rb_root *root, void *ptr)
{
	struct rb_node *node = root->rb_node;

	while (node)
	{
		heap_item_t *data = rb_entry(node, heap_item_t, node);

		if (ptr < data->ptr)
		{
			node = node->rb_left;
		}
		else if (ptr > data->ptr)
		{
			node = node->rb_right;
		}
		else
		{
			return data;
		}
	}

	return NULL;
}

static size_t heap_roundup(size_t size)
{
	static int multsize = 4 * sizeof(int);

	if (size % multsize != 0)
	{
		size += multsize - (size % multsize);
	}

	return size;
}

void *bmem_dbg_malloc(const char *file, int line, size_t size)
{
	void *ptr = NULL;
	heap_item_t *item = NULL;
	const char *base_name = get_file_name(file);
	size_t file_name_len = strlen(base_name) + 1;

	pthread_mutex_lock(&heap_mutex);

	size = heap_roundup(size);

	if ((item = malloc(sizeof(heap_item_t))) == NULL)
	{
		LOGE("memory allocation error\n");
		goto end;
	}

	memset(item, 0, sizeof(heap_item_t));

	item->size = size;

	if ((item->file = malloc(file_name_len)) == NULL)
	{
		free(item);
		LOGE("memory allocation error\n");
		goto end;
	}

	memset(item->file, 0, file_name_len);

	strcpy(item->file, base_name);

	item->line = line;

	if ((item->ptr = malloc(size + 2 * sizeof(sig_type))) == NULL)
	{
		free(item->file);
		free(item);
		LOGE("memory allocation error\n");
		goto end;
	}

	memset(item->ptr, 0, size + 2 * sizeof(sig_type));

	apply_signature(item->ptr, size);

	if (!heap_item_add(&heap_tree, item))
	{
		free(item->file);
		free(item->ptr);
		free(item);
		goto end;
	}

	// LOGD("allocating %zd bytes in heap at file %s line %d ptr %p\n", item->size, item->file, item->line, item->ptr);

	heap_state.current_size += size;

	if (heap_state.current_size > heap_state.max_size)
	{
		heap_state.max_size = heap_state.current_size;
	}

	ptr = ((sig_type *)(item->ptr)) + 1;

end:
	pthread_mutex_unlock(&heap_mutex);

	return ptr;
}

void *bmem_dbg_realloc(const char *file, int line, void *mem, size_t size)
{
	if (mem == NULL)
	{
		return bmem_dbg_malloc(file, line, size);
	}

	void *ptr = NULL;

	heap_item_t *item = NULL;

	const char *base_name = get_file_name(file);

	pthread_mutex_lock(&heap_mutex);

	item = heap_item_find(&heap_tree, ((sig_type *)mem) - 1);

	if (item == NULL)
	{
		LOGE("failed to reallocate heap item at file %s line %d\n", base_name, line);
	}
	else
	{
		void *tbptr = NULL;

		size_t file_name_len = strlen(base_name) + 1;

		rb_erase(&item->node, &heap_tree);

		check_signature(item->file, item->line, mem, item->size);

		size = heap_roundup(size);

		tbptr = realloc(item->file, file_name_len);

		if (tbptr == NULL)
		{
			heap_state.current_size -= item->size;

			free(item->file);
			free(item->ptr);
			free(item);

			LOGE("memory allocation error\n");
			goto end;
		}

		item->file = tbptr;

		memset(item->file, 0, file_name_len);

		strcpy(item->file, base_name);

		item->line = line;

		tbptr = realloc(item->ptr, size + 2 * sizeof(sig_type));

		if (tbptr == NULL)
		{
			heap_state.current_size -= item->size;

			free(item->file);
			free(item->ptr);
			free(item);

			LOGE("memory allocation [%zu] error\n", size);
			goto end;
		}

		item->ptr = tbptr;

		apply_signature(item->ptr, size);

		if (!heap_item_add(&heap_tree, item))
		{
			heap_state.current_size -= item->size;

			free(item->file);
			free(item->ptr);
			free(item);

			LOGE("memory allocation error\n");
			goto end;
		}

		heap_state.current_size += size - item->size;

		if (heap_state.current_size > heap_state.max_size)
		{
			heap_state.max_size = heap_state.current_size;
		}

		item->size = size;

		// LOGD("reallocating %zd bytes in heap at file %s line %d ptr %p\n", item->size, item->file, item->line, item->ptr);

		ptr = ((sig_type *)(item->ptr)) + 1;
	}

end:
	pthread_mutex_unlock(&heap_mutex);

	return ptr;
}

void bmem_dbg_free(const char *file, int line, void *mem)
{
	if (mem)
	{
		heap_item_t *item = NULL;

		const char *base_name = get_file_name(file);

		pthread_mutex_lock(&heap_mutex);

		item = heap_item_find(&heap_tree, ((sig_type *)mem) - 1);

		if (item == NULL)
		{
			LOGE("failed to remove heap item at file %s line %d\n", base_name, line);
		}
		else
		{
			// LOGD("freeing %zu bytes in heap at file %s line %d, heap use now %zu bytes\n",
			//      item->size, item->file, item->line, heap_state.current_size);

			check_signature(item->file, item->line, mem, item->size);

			rb_erase(&item->node, &heap_tree);

			heap_state.current_size -= item->size;

			free(item->file);
			free(item);
			free(((sig_type *)mem) - 1);
		}

		pthread_mutex_unlock(&heap_mutex);
	}
}

static void dump_mem(char *buf, int buf_len, size_t mem)
{
	if (mem < 1024)
	{
		snprintf(buf, buf_len, "(%zuBytes)", mem);
	}
	else if (mem < 1024 * 1024)
	{
		snprintf(buf, buf_len, "(%zuKB+%zuBytes)", mem / 1024,
		         mem % 1024);
	}
	else
	{
		size_t remaining_bytes = mem % (1024 * 1024);

		snprintf(buf, buf_len, "(%zuMB+%zuKB)", mem / (1024 * 1024),
		         remaining_bytes / 1024);
	}
}

void bmem_debug_dump(int verbose)
{
	trace_heap_info(0);

	char max_size_text[64];
	char current_size_text[64];

	pthread_mutex_lock(&heap_mutex);

	dump_mem(current_size_text, sizeof(current_size_text), heap_state.current_size);
	dump_mem(max_size_text, sizeof(max_size_text), heap_state.max_size);

	LOGI("== Heap Scan Results =============================\n");
	LOGI("leak:%s max:%s\n", current_size_text, max_size_text);
	LOGI("total:%zu bytes max:%zu bytes\n", heap_state.current_size, heap_state.max_size);

	if (verbose)
	{
		if (!RB_EMPTY_ROOT(&heap_tree))
		{
			heap_item_t *item = NULL;

			struct rb_root *root = &heap_tree;
			struct rb_node *current = NULL;

			for (current = rb_first(root); current; current = rb_next(current))
			{
				item = rb_entry(current, heap_item_t, node);

				// LOGI("size %-8zd line %-5d file %-32s ptr %-18p\n", item->size, item->line, item->file, item->ptr);
				LOGI("size %-8zu line %-5d file %-32s ptr %p\n", item->size, item->line, item->file, item->ptr);
#if 0
				LOGI("  content %.*s\n", item->size > 10 ? 10 : item->size, (char *)(((sig_type *)item->ptr) + 1));
#endif
			}
		}
	}

	LOGI("==================================================\n");

	pthread_mutex_unlock(&heap_mutex);
}

#else

/*
 * NOTE: totally jacked the mem alignment trick from ffmpeg, credit to them:
 *   http://www.ffmpeg.org/
 */
static void *a_malloc(size_t size)
{
	void *ptr = NULL;
#if ALIGNMENT_HACK
	long diff;
	ptr = malloc(size + ALIGNMENT);

	if (ptr)
	{
		diff = ((~(long)ptr) & (ALIGNMENT - 1)) + 1;
		ptr = (char *)ptr + diff;
		((char *)ptr)[-1] = (char)diff;
	}

#else
	ptr = malloc(size);
#endif
	return ptr;
}

static void *a_realloc(void *ptr, size_t size)
{
#if ALIGNMENT_HACK
	long diff;

	if (!ptr)
	{
		return a_malloc(size);
	}

	diff = ((char *)ptr)[-1];
	ptr = realloc((char *)ptr - diff, size + diff);

	if (ptr)
	{
		ptr = (char *)ptr + diff;
	}

	return ptr;
#else
	return realloc(ptr, size);
#endif
}

static void a_free(void *ptr)
{
#if ALIGNMENT_HACK

	if (ptr)
	{
		free((char *)ptr - ((char *)ptr)[-1]);
	}

#else
	free(ptr);
#endif
}

struct base_allocator
{
	void *(*b_malloc)(size_t);
	void *(*b_realloc)(void *, size_t);
	void (*b_free)(void *);
};

static struct base_allocator alloc = {a_malloc, a_realloc, a_free};

static volatile long num_allocs = 0;

void *bmem_malloc(size_t size)
{
	void *mem = alloc.b_malloc(size);

	if (!mem && !size)
	{
		mem = alloc.b_malloc(1);
	}

	if (!mem)
	{
		LOGE("Out of memory while trying to allocate %lu bytes\n", (unsigned long)size);
	}

	os_atomic_inc_long(&num_allocs);
	return mem;
}

void *bmem_realloc(void *mem, size_t size)
{
	if (!mem)
	{
		os_atomic_inc_long(&num_allocs);
	}

	mem = alloc.b_realloc(mem, size);

	if (!mem && !size)
	{
		mem = alloc.b_realloc(mem, 1);
	}

	if (!mem)
	{
		LOGE("Out of memory while trying to allocate %lu bytes\n", (unsigned long)size);
	}

	return mem;
}

void bmem_free(void *mem)
{
	if (mem)
	{
		os_atomic_dec_long(&num_allocs);
		alloc.b_free(mem);
	}
}

void bmem_debug_dump(int verbose)
{
	LOGI("Number of memory leaks: %ld\n", num_allocs);
}
#endif

void *bmemdup(const void *mem, size_t size)
{
	void *out = bmalloc(size);

	if (size)
	{
		memcpy(out, mem, size);
	}

	return out;
}

char *bstrndup(const char *str, size_t n)
{
	char *dup;

	if (!str)
	{
		return NULL;
	}

	dup = (char *)bmemdup(str, n + 1);
	dup[n] = 0;

	return dup;
}

char *bstrdup(const char *str)
{
	if (!str)
	{
		return NULL;
	}

	return bstrndup(str, strlen(str));
}