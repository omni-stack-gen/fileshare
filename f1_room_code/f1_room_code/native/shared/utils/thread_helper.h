#ifndef __THREADS_HELPER_H__
#define __THREADS_HELPER_H__

#include <common_defs.h>

BEGIN_C_DECLS

#include "atomic.h"

/* this may seem strange, but you can't use it unless it's an initializer */
static inline void pthread_mutex_init_value(pthread_mutex_t *mutex)
{
	pthread_mutex_t init_val = PTHREAD_MUTEX_INITIALIZER;

	if (!mutex)
	{
		return;
	}

	*mutex = init_val;
}

static inline int pthread_mutex_init_recursive(pthread_mutex_t *mutex)
{
	pthread_mutexattr_t attr;
	int ret = pthread_mutexattr_init(&attr);

	if (ret == 0)
	{
		ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

		if (ret == 0)
		{
			ret = pthread_mutex_init(mutex, &attr);
		}

		pthread_mutexattr_destroy(&attr);
	}

	return ret;
}

enum os_event_type
{
	OS_EVENT_TYPE_AUTO,
	OS_EVENT_TYPE_MANUAL,
};

struct os_event_data;
typedef struct os_event_data os_event_t;

int os_event_init(os_event_t **event, enum os_event_type type);
void os_event_destroy(os_event_t *event);
int os_event_wait(os_event_t *event);
int os_event_timedwait(os_event_t *event, unsigned long milliseconds);
int os_event_try(os_event_t *event);
int os_event_signal(os_event_t *event);
void os_event_reset(os_event_t *event);

#define OS_THREAD_NAME_MAX (16)

/**
 * Thread handle.
 */
typedef struct os_thread
{
	pthread_t handle;
} os_thread_t;

/**
 * Return value of a canceled thread.
 */
#define OS_THREAD_CANCELED PTHREAD_CANCELED

/**
 * Mutex.
 *
 * Storage space for a mutual exclusion lock.
 */
typedef pthread_mutex_t os_mutex_t;

/**
 * Static initializer for (static) mutex.
 */
#define OS_STATIC_MUTEX PTHREAD_MUTEX_INITIALIZER

/**
 * Condition variable.
 *
 * Storage space for a thread condition variable.
 */
typedef pthread_cond_t  os_cond_t;

/**
 * Static initializer for (static) condition variable.
 *
 * \note
 * The condition variable will use the default clock, which is OS-dependent.
 * Therefore, where timed waits are necessary the condition variable should
 * always be initialized dynamically explicit instead of using this
 * initializer.
 */
#define OS_STATIC_COND  PTHREAD_COND_INITIALIZER

/**
 * Semaphore.
 *
 * Storage space for a thread-safe semaphore.
 */
typedef sem_t           os_sem_t;

/**
 * Read/write lock.
 *
 * Storage space for a slim reader/writer lock.
 */
typedef pthread_rwlock_t os_rwlock_t;

/**
 * Static initializer for (static) read/write lock.
 */
#define OS_STATIC_RWLOCK PTHREAD_RWLOCK_INITIALIZER

/**
 * Thread-local key handle.
 */
typedef pthread_key_t   os_threadvar_t;

/**
 * Threaded timer handle.
 */
typedef struct os_timer *os_timer_t;

/**
 * High precision date or time interval
 *
 * Store a high precision date or time interval. The maximum precision is the
 * microsecond, and a 64 bits integer is used to avoid overflows (maximum
 * time interval is then 292271 years, which should be long enough for any
 * video). Dates are stored as microseconds since a common date (usually the
 * epoch). Note that date and time intervals can be manipulated using regular
 * arithmetic operators, and that no special functions are required.
 */
typedef int64_t os_time_t;

#define OS_TS_INVALID INT64_C(0)
#define OS_TS_0 INT64_C(1)

#define OS_CLOCK_FREQ INT64_C(1000000)

#define OS_THREAD_PRIORITY_LOW      0
#define OS_THREAD_PRIORITY_INPUT   10
#define OS_THREAD_PRIORITY_AUDIO    5
#define OS_THREAD_PRIORITY_VIDEO    0
#define OS_THREAD_PRIORITY_OUTPUT  15
#define OS_THREAD_PRIORITY_HIGHEST 20

void os_mutex_init(os_mutex_t *);

void os_mutex_init_recursive(os_mutex_t *);

void os_mutex_destroy(os_mutex_t *);

void os_mutex_lock(os_mutex_t *);

int os_mutex_trylock(os_mutex_t *) OS_USED;

void os_mutex_unlock(os_mutex_t *);

#if !defined (NDEBUG)
void os_assert_locked (os_mutex_t *);
#else
# define os_assert_locked( m ) (void)m
#endif

void os_cond_init(os_cond_t *);

void os_cond_init_daytime(os_cond_t *);

void os_cond_destroy(os_cond_t *);

int os_cond_signal(os_cond_t *);

void os_cond_broadcast(os_cond_t *);

int os_cond_wait(os_cond_t *cond, os_mutex_t *mutex);

int os_cond_timedwait(os_cond_t *cond, os_mutex_t *mutex,
                      os_time_t deadline);

int os_cond_timedwait_daytime(os_cond_t *, os_mutex_t *, time_t);

void os_sem_init(os_sem_t *, unsigned count);

void os_sem_destroy(os_sem_t *);

int os_sem_post(os_sem_t *);

void os_sem_wait(os_sem_t *);

void os_rwlock_init(os_rwlock_t *);

void os_rwlock_destroy(os_rwlock_t *);

void os_rwlock_rdlock(os_rwlock_t *);

void os_rwlock_wrlock(os_rwlock_t *);

void os_rwlock_unlock(os_rwlock_t *);

int os_threadvar_create(os_threadvar_t *key, void (*destr)(void *));

void os_threadvar_delete(os_threadvar_t *);

int os_threadvar_set(os_threadvar_t key, void *value);

void *os_threadvar_get(os_threadvar_t);

int os_thread_create(os_thread_t *th, void *(*entry)(void *), void *data,
                     int priority) OS_USED;

void os_thread_cancel(os_thread_t);

void os_thread_join(os_thread_t th, void **result);

int os_thread_set_name(const char *name);

int os_thread_savecancel(void);

void os_thread_restorecancel(int state);

void os_thread_control_cancel(int cmd, ...);

int os_thread_create_detach(os_thread_t *th, void *(*entry)(void *), void *data,
                            int priority);

os_thread_t os_thread_self(void) OS_USED;

unsigned long os_thread_id(void) OS_USED;

os_time_t os_date(void);

uint64_t os_gettime_ns(void);

bool os_sleepto_ns(uint64_t time_target);
// bool os_sleepto_ns_fast(uint64_t time_target);
#define os_sleepto_ns_fast(a) _os_sleepto_ns_fast(a, __FUNCTION__, __LINE__)
bool _os_sleepto_ns_fast(uint64_t time_target, const char *func, const long line);
void os_sleep_ms(uint32_t duration);

int os_timer_create(os_timer_t *id, void (*func)(void *), void *data) OS_USED;

void os_timer_destroy(os_timer_t timer);

void os_timer_schedule(os_timer_t timer, bool absolute, os_time_t value, os_time_t interval);

unsigned os_timer_getoverrun(os_timer_t) OS_USED;

enum
{
	OS_CLEANUP_PUSH,
	OS_CLEANUP_POP,
	OS_CANCEL_ADDR_SET,
	OS_CANCEL_ADDR_CLEAR,
};

#define os_cleanup_push( routine, arg ) pthread_cleanup_push (routine, arg)

/**
 * Unregisters the last cancellation handler.
 *
 * This pops the cancellation handler that was last pushed with
 * os_cleanup_push() in the calling thread.
 */
#define os_cleanup_pop( ) pthread_cleanup_pop (0)

static inline void os_cleanup_lock(void *lock)
{
	os_mutex_unlock((os_mutex_t *)lock);
}
#define mutex_cleanup_push( lock ) os_cleanup_push (os_cleanup_lock, lock)

static inline void os_thread_cancel_addr_set(void *addr)
{
	os_thread_control_cancel(OS_CANCEL_ADDR_SET, addr);
}

static inline void os_thread_cancel_addr_clear(void *addr)
{
	os_thread_control_cancel(OS_CANCEL_ADDR_CLEAR, addr);
}

END_C_DECLS

#ifdef __cplusplus
/**
 * Helper C++ class to lock a mutex.
 *
 * The mutex is locked when the object is created, and unlocked when the object
 * is destroyed.
 */
class os_mutex_locker
{
	private:
		os_mutex_t *lock;
	public:
		os_mutex_locker(os_mutex_t *m) : lock(m)
		{
			os_mutex_lock(lock);
		}

		~os_mutex_locker(void)
		{
			os_mutex_unlock(lock);
		}
};
#endif

#endif /* __THREADS_HELPER_H__ */