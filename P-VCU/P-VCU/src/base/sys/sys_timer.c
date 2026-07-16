#include <common_defs.h>

#include <stdnoreturn.h>
#include <stdatomic.h>

#include <errno.h>
#include <assert.h>

#include <sys/sys_timer.h>

#include <utils/bmem.h>
#include <utils/util.h>

mtime_t mdate(void)
{
	struct timespec ts;

	if (unlikely(clock_gettime(CLOCK_MONOTONIC, &ts) != 0))
	{
		abort();
	}

	return (INT64_C(1000000) * ts.tv_sec) + (ts.tv_nsec / 1000);
}

static struct timespec mtime_to_ts(mtime_t date)
{
	lldiv_t d = lldiv(date, CLOCK_FREQ);
	struct timespec ts = { d.quot, d.rem *(1000000000 / CLOCK_FREQ) };

	return ts;
}

/////////////////////////////////////////////////////////////////

static inline void cleanup_lock(void *lock)
{
	pthread_mutex_unlock((pthread_mutex_t *)lock);
}
#define mutex_cleanup_push( lock ) pthread_cleanup_push (cleanup_lock, lock)

static int thread_savecancel(void)
{
	int state;
	int val = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state);

	(void)val;
	return state;
}

static void thread_restorecancel(int state)
{
	pthread_setcancelstate(state, NULL);
}

static void cond_init(pthread_cond_t *p_condvar)
{
	pthread_condattr_t attr;

	if (unlikely(pthread_condattr_init(&attr)))
	{
		abort();
	}

	pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);

	if (unlikely(pthread_cond_init(p_condvar, &attr)))
	{
		abort();
	}

	pthread_condattr_destroy(&attr);
}

static int cond_timedwait(pthread_cond_t *p_condvar, pthread_mutex_t *p_mutex,
                          mtime_t deadline)
{
	struct timespec ts = mtime_to_ts(deadline);
	int val = pthread_cond_timedwait(p_condvar, p_mutex, &ts);

	// if (val != ETIMEDOUT)
	// {
	//  MEDIA_THREAD_ASSERT("timed-waiting on condition");
	// }

	return val;
}

static void mutex_init(pthread_mutex_t *p_mutex)
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

/////////////////////////////////////////////////////////////////

/*
 * POSIX timers are essentially unusable from a library: there provide no safe
 * way to ensure that a timer has no pending/ongoing iteration. Furthermore,
 * they typically require one thread per timer plus one thread per iteration,
 * which is inefficient and overkill (unless you need multiple iteration
 * of the same timer concurrently).
 * Thus, this is a generic manual implementation of timers using a thread.
 */

struct sys_timer
{
	pthread_t thread;
	pthread_cond_t reschedule;
	pthread_mutex_t lock;
	void (*func)(void *);
	void *data;
	uint64_t value, interval;
	atomic_uint overruns;
};

noreturn static void *timer_thread(void *data)
{
	struct sys_timer *timer = data;

	pthread_mutex_lock(&timer->lock);
	mutex_cleanup_push(&timer->lock);

	for (;;)
	{
		while (timer->value == 0)
		{
			assert(timer->interval == 0);
			printf("timer_thread: disarmed\n");
			pthread_cond_wait(&timer->reschedule, &timer->lock);
			printf("timer_thread: reawakened\n");
		}

		if (timer->interval != 0)
		{
			mtime_t now = mdate();

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

		mtime_t value = timer->value;

		if (cond_timedwait(&timer->reschedule, &timer->lock, value) == 0)
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

		pthread_mutex_unlock(&timer->lock);

		int canc = thread_savecancel();
		timer->func(timer->data);
		thread_restorecancel(canc);

		pthread_mutex_lock(&timer->lock);
	}

	pthread_cleanup_pop(0);
	// assert_unreachable();
}

int sys_timer_create(sys_timer_t *id, void (*func)(void *), void *data)
{
	struct sys_timer *timer = bmalloc(sizeof(*timer));

	if (unlikely(timer == NULL))
	{
		return ENOMEM;
	}

	memset(timer, 0, sizeof(struct sys_timer));

	mutex_init(&timer->lock);
	cond_init(&timer->reschedule);
	assert(func);
	timer->func = func;
	timer->data = data;
	timer->value = 0;
	timer->interval = 0;
	atomic_init(&timer->overruns, 0);

	if (pthread_create(&timer->thread, NULL, timer_thread, timer))
	{
		pthread_cond_destroy(&timer->reschedule);
		pthread_mutex_destroy(&timer->lock);
		bfree(timer);
		return ENOMEM;
	}

	*id = timer;
	return 0;
}

void sys_timer_destroy(sys_timer_t timer)
{
	pthread_cancel(timer->thread);
	pthread_join(timer->thread, NULL);
	pthread_cond_destroy(&timer->reschedule);
	pthread_mutex_destroy(&timer->lock);
	bfree(timer);
}

void sys_timer_schedule(sys_timer_t timer, bool absolute, mtime_t value, mtime_t interval)
{
	if (value == 0)
	{
		interval = 0;
	}
	else if (!absolute)
	{
		value += mdate();
	}

	pthread_mutex_lock(&timer->lock);
	timer->value = value;
	timer->interval = interval;
	pthread_cond_signal(&timer->reschedule);
	pthread_mutex_unlock(&timer->lock);
}

unsigned sys_timer_getoverrun(sys_timer_t timer)
{
	return atomic_exchange_explicit(&timer->overruns, 0,
	                                memory_order_relaxed);
}
