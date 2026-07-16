#include <utils/bmem.h>
#include <utils/rb_tree.h>
#include <utils/atomic.h>

#include <pthread.h>

#define LOG_TAG "bmem"
#include <log.h>

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
		LOGE("invalid start signature 0x%llx in heap item at file %s line %d\n",
		     *(sig_type *)(mem - SIG_SIZE), file, line);
	}

	sig = SIG_END;

	if (0 != memcmp(mem + size, &sig, SIG_SIZE))
	{
		LOGE("invalid end signature 0x%llx in heap item at file %s line %d\n",
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
	heap_item_t *item = NULL;
	const char *base_name = get_file_name(file);
	size_t file_name_len = strlen(base_name) + 1;
	void *ptr = NULL;

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

	//printf("allocating %zd bytes in heap at file %s line %d ptr %p\n", item->size, item->file, item->line, item->ptr);

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

	heap_item_t *item = NULL;
	void *ptr = NULL;

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

			LOGE("memory allocation error\n");
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
			// LOGD("freeing %zd bytes in heap at file %s line %d, heap use now %zd bytes\n",
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

void bmem_debug_dump()
{
	pthread_mutex_lock(&heap_mutex);

	char current_size_text[64];

	if (heap_state.current_size < 1024)
	{
		snprintf(current_size_text, sizeof(current_size_text) - 1, "%zdBytes", heap_state.current_size);
	}
	else if (heap_state.current_size < 1024 * 1024)
	{
		snprintf(current_size_text, sizeof(current_size_text) - 1, "(%zdKB+%zdBytes)", heap_state.current_size / 1024,
		         heap_state.current_size % 1024);
	}
	else
	{
		snprintf(current_size_text, sizeof(current_size_text) - 1, "(%zdMB+%zdKB)", heap_state.current_size / (1024 * 1024),
		         heap_state.current_size / (1024 * 1024));
	}

	char max_size_text[64];

	if (heap_state.max_size < 1024)
	{
		snprintf(max_size_text, sizeof(max_size_text) - 1, "(%zdBytes)", heap_state.max_size);
	}
	else if (heap_state.max_size < 1024 * 1024)
	{
		snprintf(max_size_text, sizeof(max_size_text) - 1, "(%zdKB+%zdBytes)", heap_state.max_size / 1024,
		         heap_state.max_size % 1024);
	}
	else
	{
		snprintf(max_size_text, sizeof(max_size_text) - 1, "(%zdMB+%zdKB)", heap_state.max_size / (1024 * 1024),
		         heap_state.max_size % (1024 * 1024));
	}

	LOGI("== Heap Scan Results =============================\n");
	LOGI("leak:%s max:%s\n", current_size_text, max_size_text);
	LOGI("total:%d bytes max:%d bytes\n", heap_state.current_size, heap_state.max_size);

#if 1

	if (!RB_EMPTY_ROOT(&heap_tree))
	{
		heap_item_t *item = NULL;

		struct rb_root *root = &heap_tree;
		struct rb_node *current = NULL;

		for (current = rb_first(root); current; current = rb_next(current))
		{
			item = rb_entry(current, heap_item_t, node);

			LOGI("size %zd, line %d, file %s, ptr %p\n", item->size, item->line, item->file, item->ptr);
#if 0
			LOGI("  content %.*s\n", item->size > 10 ? 10 : item->size, (char *)(((sig_type *)item->ptr) + 1));
#endif
		}
	}

	LOGI("==================================================\n");
#endif

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

	atomic_inc_long(&num_allocs);
	return mem;
}

void *bmem_realloc(void *mem, size_t size)
{
	if (!mem)
	{
		atomic_inc_long(&num_allocs);
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
		atomic_dec_long(&num_allocs);
		alloc.b_free(mem);
	}
}

void bmem_debug_dump()
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