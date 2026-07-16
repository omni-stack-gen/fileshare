#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <pthread.h>
#include <execinfo.h>

#include <stdnoreturn.h>
#include <stdatomic.h>

#include <utils/thread_helper.h>
#include <utils/util.h>
#include <utils/bmem.h>

#define LOG_TAG "thread_helper"
#include <utils/log.h>

bool os_sleepto_ns(uint64_t time_target)
{
	uint64_t current = os_gettime_ns();

	if (time_target < current)
	{
		return false;
	}

	time_target -= current;

	struct timespec req, remain;
	memset(&req, 0, sizeof(req));
	memset(&remain, 0, sizeof(remain));
	req.tv_sec = time_target / 1000000000;
	req.tv_nsec = time_target % 1000000000;

	while (nanosleep(&req, &remain))
	{
		req = remain;
		memset(&remain, 0, sizeof(remain));
	}

	return true;
}

bool _os_sleepto_ns_fast(uint64_t time_target, const char *func, const long line)
{
	uint64_t current = os_gettime_ns();

	if (time_target < current)
	{
		// LOGE("[%s():%ld] os_sleepto_ns_fast error \n", func, line);
		// cpu时间有时候不太准确，sleep 1个毫秒让出cpu，防止线程一直跑无法让出cpu
		usleep(1000);
		return false;
	}

	do
	{
		uint64_t remain_us = (time_target - current + 999) / 1000;
		useconds_t us = remain_us >= 1000000 ? 999999 : remain_us;
		usleep(us);

		current = os_gettime_ns();
	} while (time_target > current);

	return true;
}

void os_sleep_ms(uint32_t duration)
{
#if 0
	usleep(duration * 1000);
#else
	struct timespec request, remaining;

	request.tv_sec = duration / 1000;
	request.tv_nsec = (duration % 1000) * 1000000;

	while (nanosleep(&request, &remaining) == -1 && errno == EINTR)
	{
		request = remaining;
	}

#endif
}

struct os_event_data
{
	os_mutex_t mutex;
	os_cond_t cond;
	volatile bool signalled;
	bool manual;
};

int os_event_init(os_event_t **event, enum os_event_type type)
{
	int code = 0;

	struct os_event_data *data = bzalloc(sizeof(struct os_event_data));

	os_mutex_init(&data->mutex);
	os_cond_init(&data->cond);

	data->manual = (type == OS_EVENT_TYPE_MANUAL);
	data->signalled = false;
	*event = data;

	return 0;
}

void os_event_destroy(os_event_t *event)
{
	if (event)
	{
		os_mutex_destroy(&event->mutex);
		os_cond_destroy(&event->cond);
		bfree(event);
	}
}

int os_event_wait(os_event_t *event)
{
	int code = 0;

	os_mutex_lock(&event->mutex);

	while (!event->signalled)
	{
		code = os_cond_wait(&event->cond, &event->mutex);

		if (code != 0)
		{
			break;
		}
	}

	if (code == 0)
	{
		if (!event->manual)
		{
			event->signalled = false;
		}
	}

	os_mutex_unlock(&event->mutex);

	return code;
}

int os_event_timedwait(os_event_t *event, unsigned long milliseconds)
{
	int code = 0;

	os_time_t timeout;

	os_mutex_lock(&event->mutex);

	while (!event->signalled)
	{
		timeout = os_date() + milliseconds * 1000;

		code = os_cond_timedwait(&event->cond, &event->mutex, timeout);

		if (code != 0)
		{
			break;
		}
	}

	if (code == 0)
	{
		if (!event->manual)
		{
			event->signalled = false;
		}
	}

	os_mutex_unlock(&event->mutex);

	return code;
}

int os_event_try(os_event_t *event)
{
	int ret = EAGAIN;

	os_mutex_lock(&event->mutex);

	if (event->signalled)
	{
		if (!event->manual)
		{
			event->signalled = false;
		}

		ret = 0;
	}

	os_mutex_unlock(&event->mutex);

	return ret;
}

int os_event_signal(os_event_t *event)
{
	int code = 0;

	os_mutex_lock(&event->mutex);
	code = os_cond_signal(&event->cond);
	event->signalled = true;
	os_mutex_unlock(&event->mutex);

	return code;
}

void os_event_reset(os_event_t *event)
{
	os_mutex_lock(&event->mutex);
	event->signalled = false;
	os_mutex_unlock(&event->mutex);
}

void os_set_thread_name(const char *name)
{
	// prctl(PR_SET_NAME, name);

	if (strlen(name) <= 15)
	{
		pthread_setname_np(pthread_self(), name);
	}
	else
	{
		char *thread_name = bstrndup(name, 15);
		pthread_setname_np(pthread_self(), thread_name);
		bfree(thread_name);
	}
}

/* ------------------------------------------------------------------------- */

#if !defined (_POSIX_TIMERS)
	#define _POSIX_TIMERS (-1)
#endif
#if !defined (_POSIX_CLOCK_SELECTION)
	/* Clock selection was defined in 2001 and became mandatory in 2008. */
	#define _POSIX_CLOCK_SELECTION (-1)
#endif
#if !defined (_POSIX_MONOTONIC_CLOCK)
	#define _POSIX_MONOTONIC_CLOCK (-1)
#endif

#if (_POSIX_TIMERS > 0)
static unsigned os_clock_prec;

#if (_POSIX_MONOTONIC_CLOCK > 0) && (_POSIX_CLOCK_SELECTION > 0)
	/* Compile-time POSIX monotonic clock support */
	#define os_clock_id (CLOCK_MONOTONIC)

#elif (_POSIX_MONOTONIC_CLOCK == 0) && (_POSIX_CLOCK_SELECTION > 0)
	/* Run-time POSIX monotonic clock support (see clock_setup() below) */
	static clockid_t os_clock_id;

#else
	/* No POSIX monotonic clock support */
	#define os_clock_id (CLOCK_REALTIME)
	#   warning Monotonic clock not available. Expect timing issues.

#endif /* _POSIX_MONOTONIC_CLOKC */

static void os_clock_setup_once(void)
{
# if (_POSIX_MONOTONIC_CLOCK == 0)
	long val = sysconf(_SC_MONOTONIC_CLOCK);
	assert(val != 0);
	os_clock_id = (val < 0) ? CLOCK_REALTIME : CLOCK_MONOTONIC;
# endif

	struct timespec res;

	if (unlikely(clock_getres(os_clock_id, &res) != 0 || res.tv_sec != 0))
	{
		abort();
	}

	os_clock_prec = (res.tv_nsec + 500) / 1000;
}

static pthread_once_t os_clock_once = PTHREAD_ONCE_INIT;

# define os_clock_setup() \
	pthread_once(&os_clock_once, os_clock_setup_once)

#else /* _POSIX_TIMERS */

# include <sys/time.h> /* gettimeofday() */

# define os_clock_setup() (void)0
# warning Monotonic clock not available. Expect timing issues.
#endif /* _POSIX_TIMERS */

static struct timespec os_time_to_ts(os_time_t date)
{
	lldiv_t d = lldiv(date, OS_CLOCK_FREQ);
	struct timespec ts = { d.quot, d.rem *(1000000000 / OS_CLOCK_FREQ) };

	return ts;
}

/**
 * Print a backtrace to the standard error for debugging purpose.
 */
void os_trace(const char *fn, const char *file, unsigned line)
{
	fprintf(stderr, "at %s:%u in %s\n", file, line, fn);
	fflush(stderr);  /* needed before switch to low-level I/O */
	// #ifdef HAVE_BACKTRACE
	void *stack[20];
	int len = backtrace(stack, sizeof(stack) / sizeof(stack[0]));
	backtrace_symbols_fd(stack, len, 2);
	// #endif
	fsync(2);
}

#ifndef NDEBUG
/**
 * Reports a fatal error from the threading layer, for debugging purposes.
 */
static void
os_thread_fatal(const char *action, int error,
                const char *function, const char *file, unsigned line)
{
	int canc = os_thread_savecancel();
	fprintf(stderr, "fatal error %s (%d) in thread %lu ",
	        action, error, os_thread_id());
	os_trace(function, file, line);
	perror("Thread error");
	fflush(stderr);

	os_thread_restorecancel(canc);
	abort();
}

# define OS_THREAD_ASSERT( action ) \
	if (unlikely(val)) \
		os_thread_fatal (action, val, __func__, __FILE__, __LINE__)
#else
# define OS_THREAD_ASSERT( action ) ((void)val)
#endif

void os_mutex_init(os_mutex_t *p_mutex)
{
	pthread_mutexattr_t attr;

	if (unlikely(pthread_mutexattr_init(&attr)))
	{
		abort();
	}

#ifdef NDEBUG
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_DEFAULT);
#else
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#endif

	if (unlikely(pthread_mutex_init(p_mutex, &attr)))
	{
		abort();
	}

	pthread_mutexattr_destroy(&attr);
}

void os_mutex_init_recursive(os_mutex_t *p_mutex)
{
	pthread_mutexattr_t attr;

	if (unlikely(pthread_mutexattr_init(&attr)))
	{
		abort();
	}

	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

	if (unlikely(pthread_mutex_init(p_mutex, &attr)))
	{
		abort();
	}

	pthread_mutexattr_destroy(&attr);
}

void os_mutex_destroy(os_mutex_t *p_mutex)
{
	int val = pthread_mutex_destroy(p_mutex);
	OS_THREAD_ASSERT("destroying mutex");
}

void os_mutex_lock(os_mutex_t *p_mutex)
{
	int val = pthread_mutex_lock(p_mutex);
	OS_THREAD_ASSERT("locking mutex");
}

int os_mutex_trylock(os_mutex_t *p_mutex)
{
	int val = pthread_mutex_trylock(p_mutex);

	if (val != EBUSY)
	{
		OS_THREAD_ASSERT("locking mutex");
	}

	return val;
}

void os_mutex_unlock(os_mutex_t *p_mutex)
{
	int val = pthread_mutex_unlock(p_mutex);
	OS_THREAD_ASSERT("unlocking mutex");
}

#ifndef NDEBUG
/**
 * Asserts that a mutex is locked by the calling thread.
 */
void os_assert_locked (os_mutex_t *p_mutex)
{
    assert (pthread_mutex_lock (p_mutex) == EDEADLK);
}
#endif

void os_cond_init(os_cond_t *p_condvar)
{
	pthread_condattr_t attr;

	if (unlikely(pthread_condattr_init(&attr)))
	{
		abort();
	}

#if (_POSIX_CLOCK_SELECTION > 0)
	os_clock_setup();
	pthread_condattr_setclock(&attr, os_clock_id);
#endif

	if (unlikely(pthread_cond_init(p_condvar, &attr)))
	{
		abort();
	}

	pthread_condattr_destroy(&attr);
}

void os_cond_init_daytime(os_cond_t *p_condvar)
{
	if (unlikely(pthread_cond_init(p_condvar, NULL)))
	{
		abort();
	}
}

void os_cond_destroy(os_cond_t *p_condvar)
{
	int val = pthread_cond_destroy(p_condvar);
	OS_THREAD_ASSERT("destroying condition");
}

int os_cond_signal(os_cond_t *p_condvar)
{
	int val = pthread_cond_signal(p_condvar);
	OS_THREAD_ASSERT("signaling condition variable");
	return val;
}

void os_cond_broadcast(os_cond_t *p_condvar)
{
	pthread_cond_broadcast(p_condvar);
}

int os_cond_wait(os_cond_t *p_condvar, os_mutex_t *p_mutex)
{
	int val = pthread_cond_wait(p_condvar, p_mutex);
	OS_THREAD_ASSERT("waiting on condition");

	return val;
}

int os_cond_timedwait(os_cond_t *p_condvar, os_mutex_t *p_mutex,
                      os_time_t deadline)
{
	struct timespec ts = os_time_to_ts(deadline);
	int val = pthread_cond_timedwait(p_condvar, p_mutex, &ts);

	if (val != ETIMEDOUT)
	{
		OS_THREAD_ASSERT("timed-waiting on condition");
	}

	return val;
}

int os_cond_timedwait_daytime(os_cond_t *p_condvar, os_mutex_t *p_mutex,
                              time_t deadline)
{
	struct timespec ts = { deadline, 0 };
	int val = pthread_cond_timedwait(p_condvar, p_mutex, &ts);

	if (val != ETIMEDOUT)
	{
		OS_THREAD_ASSERT("timed-waiting on condition");
	}

	return val;
}

void os_sem_init(os_sem_t *sem, unsigned value)
{
	if (unlikely(sem_init(sem, 0, value)))
	{
		abort();
	}
}

void os_sem_destroy(os_sem_t *sem)
{
	int val;

	if (likely(sem_destroy(sem) == 0))
	{
		return;
	}

	val = errno;

	OS_THREAD_ASSERT("destroying semaphore");
}

int os_sem_post(os_sem_t *sem)
{
	int val;

	if (likely(sem_post(sem) == 0))
	{
		return 0;
	}

	val = errno;

	if (unlikely(val != EOVERFLOW))
	{
		OS_THREAD_ASSERT("unlocking semaphore");
	}

	return val;
}

void os_sem_wait(os_sem_t *sem)
{
	int val;

	do
	{
		if (likely(sem_wait(sem) == 0))
		{
			return;
		}
	} while ((val = errno) == EINTR);

	OS_THREAD_ASSERT("locking semaphore");
}

void os_rwlock_init(os_rwlock_t *lock)
{
	if (unlikely(pthread_rwlock_init(lock, NULL)))
	{
		abort();
	}
}

void os_rwlock_destroy(os_rwlock_t *lock)
{
	int val = pthread_rwlock_destroy(lock);
	OS_THREAD_ASSERT("destroying R/W lock");
}

void os_rwlock_rdlock(os_rwlock_t *lock)
{
	int val = pthread_rwlock_rdlock(lock);
	OS_THREAD_ASSERT("acquiring R/W lock for reading");
}

void os_rwlock_wrlock(os_rwlock_t *lock)
{
	int val = pthread_rwlock_wrlock(lock);
	OS_THREAD_ASSERT("acquiring R/W lock for writing");
}

void os_rwlock_unlock(os_rwlock_t *lock)
{
	int val = pthread_rwlock_unlock(lock);
	OS_THREAD_ASSERT("releasing R/W lock");
}

int os_threadvar_create(os_threadvar_t *key, void (*destr)(void *))
{
	return pthread_key_create(key, destr);
}

void os_threadvar_delete(os_threadvar_t *p_tls)
{
	pthread_key_delete(*p_tls);
}

int os_threadvar_set(os_threadvar_t key, void *value)
{
	return pthread_setspecific(key, value);
}

void *os_threadvar_get(os_threadvar_t key)
{
	return pthread_getspecific(key);
}

static bool rt_priorities = false;
static int rt_offset = 0;

static int os_thread_create_attr(os_thread_t *th, pthread_attr_t *attr,
                                 void *(*entry)(void *), void *data, int priority)
{
	int ret;

#if 0
	/* Block the signals that signals interface plugin handles.
	 * If the LibVLC caller wants to handle some signals by itself, it should
	 * block these before whenever invoking LibVLC. And it must obviously not
	 * start the VLC signals interface plugin.
	 *
	 * LibVLC will normally ignore any interruption caused by an asynchronous
	 * signal during a system call. But there may well be some buggy cases
	 * where it fails to handle EINTR (bug reports welcome). Some underlying
	 * libraries might also not handle EINTR properly.
	 */
	sigset_t oldset;
	{
		sigset_t set;
		sigemptyset(&set);
		sigdelset(&set, SIGHUP);
		sigaddset(&set, SIGINT);
		sigaddset(&set, SIGQUIT);
		sigaddset(&set, SIGTERM);

		sigaddset(&set, SIGPIPE);  /* We don't want this one, really! */
		pthread_sigmask(SIG_BLOCK, &set, &oldset);
	}
#endif

#if defined (_POSIX_PRIORITY_SCHEDULING) && (_POSIX_PRIORITY_SCHEDULING >= 0) \
 && defined (_POSIX_THREAD_PRIORITY_SCHEDULING) \
 && (_POSIX_THREAD_PRIORITY_SCHEDULING >= 0)

	if (rt_priorities)
	{
		struct sched_param sp = { .sched_priority = priority + rt_offset, };
		int policy;

		if (sp.sched_priority <= 0)
		{
			sp.sched_priority += sched_get_priority_max(policy = SCHED_OTHER);
		}
		else
		{
			sp.sched_priority += sched_get_priority_min(policy = SCHED_RR);
		}

		pthread_attr_setschedpolicy(attr, policy);
		pthread_attr_setschedparam(attr, &sp);
		pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
	}

#else
	(void) priority;
#endif

	/* The thread stack size.
	 * The lower the value, the less address space per thread, the highest
	 * maximum simultaneous threads per process. Too low values will cause
	 * stack overflows and weird crashes. Set with caution. Also keep in mind
	 * that 64-bits platforms consume more stack than 32-bits one.
	 *
	 * Thanks to on-demand paging, thread stack size only affects address space
	 * consumption. In terms of memory, threads only use what they need
	 * (rounded up to the page boundary).
	 *
	 * For example, on Linux i386, the default is 2 mega-bytes, which supports
	 * about 320 threads per processes. */
#define OS_STACKSIZE (128 * sizeof (void *) * 1024)

#ifdef OS_STACKSIZE
	ret = pthread_attr_setstacksize(attr, OS_STACKSIZE);
	assert(ret == 0);  /* fails iif OS_STACKSIZE is invalid */
#endif

	ret = pthread_create(&th->handle, attr, entry, data);
#if 0
	pthread_sigmask(SIG_SETMASK, &oldset, NULL);
#endif
	pthread_attr_destroy(attr);
	return ret;
}

int os_thread_create(os_thread_t *th, void *(*entry)(void *), void *data,
                     int priority)
{
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	return os_thread_create_attr(th, &attr, entry, data, priority);
}

void os_thread_join(os_thread_t th, void **result)
{
	int val = pthread_join(th.handle, result);
	OS_THREAD_ASSERT("joining thread");
}

int os_thread_set_name(const char *name)
{
#ifdef __linux__
	return pthread_setname_np(pthread_self(), name);
#else
	return prctl(PR_SET_NAME, name);
#endif
}

int os_thread_create_detach(os_thread_t *th, void *(*entry)(void *), void *data,
                            int priority)
{
	os_thread_t dummy;
	pthread_attr_t attr;

	if (th == NULL)
	{
		th = &dummy;
	}

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	return os_thread_create_attr(th, &attr, entry, data, priority);
}

os_thread_t os_thread_self(void)
{
	os_thread_t thread = { pthread_self() };
	return thread;
}

unsigned long os_thread_id(void)
{
	static __thread pid_t tid = 0;

	if (unlikely(tid == 0))
	{
		tid = syscall(__NR_gettid);
	}

	return tid;
}

int os_set_priority(os_thread_t th, int priority)
{
#if defined (_POSIX_PRIORITY_SCHEDULING) && (_POSIX_PRIORITY_SCHEDULING >= 0) \
 && defined (_POSIX_THREAD_PRIORITY_SCHEDULING) \
 && (_POSIX_THREAD_PRIORITY_SCHEDULING >= 0)

	if (rt_priorities)
	{
		struct sched_param sp = { .sched_priority = priority + rt_offset, };
		int policy;

		if (sp.sched_priority <= 0)
		{
			sp.sched_priority += sched_get_priority_max(policy = SCHED_OTHER);
		}
		else
		{
			sp.sched_priority += sched_get_priority_min(policy = SCHED_RR);
		}

		if (pthread_setschedparam(th.handle, policy, &sp))
		{
			return -1;
		}
	}

#else
	(void) th;
	(void) priority;
#endif
	return 0;
}

void os_thread_cancel(os_thread_t th)
{
	pthread_cancel(th.handle);
}

int os_thread_savecancel(void)
{
	int state;
	int val = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state);

	OS_THREAD_ASSERT("saving cancellation");
	return state;
}

void os_thread_restorecancel(int state)
{
#ifndef NDEBUG
	int oldstate, val;

	val = pthread_setcancelstate(state, &oldstate);
	/* This should fail if an invalid value for given for state */
	OS_THREAD_ASSERT("restoring cancellation");

	if (unlikely(oldstate != PTHREAD_CANCEL_DISABLE))
		os_thread_fatal("restoring cancellation while not disabled", EINVAL,
		                __func__, __FILE__, __LINE__);

#else
	pthread_setcancelstate(state, NULL);
#endif
}

void os_testcancel(void)
{
	pthread_testcancel();
}

void os_thread_control_cancel(int cmd, ...)
{
	(void) cmd;
	os_assert_unreachable();
}

os_time_t os_date(void)
{
#if (_POSIX_TIMERS > 0)
	struct timespec ts;

	os_clock_setup();

	if (unlikely(clock_gettime(os_clock_id, &ts) != 0))
	{
		abort();
	}

	return (INT64_C(1000000) * ts.tv_sec) + (ts.tv_nsec / 1000);

#else
	struct timeval tv;

	if (unlikely(gettimeofday(&tv, NULL) != 0))
	{
		abort();
	}

	return (INT64_C(1000000) * tv.tv_sec) + tv.tv_usec;

#endif
}

uint64_t os_gettime_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec);
}

/*
 * POSIX timers are essentially unusable from a library: there provide no safe
 * way to ensure that a timer has no pending/ongoing iteration. Furthermore,
 * they typically require one thread per timer plus one thread per iteration,
 * which is inefficient and overkill (unless you need multiple iteration
 * of the same timer concurrently).
 * Thus, this is a generic manual implementation of timers using a thread.
 */

struct os_timer
{
	os_thread_t thread;
	os_cond_t reschedule;
	os_mutex_t lock;
	void (*func)(void *);
	void *data;
	uint64_t value, interval;
	atomic_uint overruns;
};

noreturn static void *os_timer_thread(void *data)
{
	struct os_timer *timer = data;

	char name[OS_THREAD_NAME_MAX];

	snprintf(name, sizeof(name), "timer%p", data);

	os_thread_set_name(name);

	os_mutex_lock(&timer->lock);
	mutex_cleanup_push(&timer->lock);

	for (;;)
	{
		while (timer->value == 0)
		{
			assert(timer->interval == 0);
			os_cond_wait(&timer->reschedule, &timer->lock);
		}

		if (timer->interval != 0)
		{
			os_time_t now = os_date();

			if (now > timer->value)
			{
				/* Update overrun counter */
				unsigned misses = (now - timer->value) / timer->interval;

				timer->value += misses * timer->interval;
				assert(timer->value <= now);
				atomic_fetch_add_explicit(&timer->overruns, misses,
				                          memory_order_relaxed);
			}
		}

		os_time_t value = timer->value;

		if (os_cond_timedwait(&timer->reschedule, &timer->lock, value) == 0)
		{
			continue;
		}

		if (likely(timer->value <= value))
		{
			timer->value += timer->interval; /* rearm */

			if (timer->interval == 0)
			{
				timer->value = 0;    /* disarm */
			}
		}

		os_mutex_unlock(&timer->lock);

		int canc = os_thread_savecancel();
		timer->func(timer->data);
		os_thread_restorecancel(canc);

		os_mutex_lock(&timer->lock);
	}

	os_cleanup_pop();
	os_assert_unreachable();
}

int os_timer_create(os_timer_t *id, void (*func)(void *), void *data)
{
	struct os_timer *timer = bmalloc(sizeof(*timer));

	if (unlikely(timer == NULL))
	{
		return ENOMEM;
	}

	os_mutex_init(&timer->lock);
	os_cond_init(&timer->reschedule);
	assert(func);
	timer->func = func;
	timer->data = data;
	timer->value = 0;
	timer->interval = 0;
	atomic_init(&timer->overruns, 0);

	if (os_thread_create(&timer->thread, os_timer_thread, timer,
	                     OS_THREAD_PRIORITY_INPUT))
	{
		os_cond_destroy(&timer->reschedule);
		os_mutex_destroy(&timer->lock);
		bfree(timer);
		return ENOMEM;
	}

	*id = timer;
	return 0;
}

void os_timer_destroy(os_timer_t timer)
{
	os_thread_cancel(timer->thread);
	os_thread_join(timer->thread, NULL);
	os_cond_destroy(&timer->reschedule);
	os_mutex_destroy(&timer->lock);
	bfree(timer);
}

void os_timer_schedule(os_timer_t timer, bool absolute, os_time_t value, os_time_t interval)
{
	if (value == 0)
	{
		interval = 0;
	}
	else if (!absolute)
	{
		value += os_date();
	}

	pthread_mutex_lock(&timer->lock);
	timer->value = value;
	timer->interval = interval;
	pthread_cond_signal(&timer->reschedule);
	pthread_mutex_unlock(&timer->lock);
}

unsigned os_timer_getoverrun(os_timer_t timer)
{
	return atomic_exchange_explicit(&timer->overruns, 0,
	                                memory_order_relaxed);
}