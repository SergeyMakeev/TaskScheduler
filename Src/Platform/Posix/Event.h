#pragma once

#include <sys/time.h>
#include <sched.h>
#include <errno.h>


namespace MT
{
	//
	//
	//
	class Event
	{
		static const int NOT_SIGNALED = 0;
		static const int SIGNALED = 1;

		pthread_mutex_t	mutex;
		pthread_cond_t	condition;
		AtomicInt val;
		EventReset::Type resetType;
		bool isInitialized;

	private:

		Event(const Event&) {}
		void operator=(const Event&) {}

		void AutoResetIfNeed()
		{
			if (resetType == EventReset::MANUAL)
			{
				return;
			}
			Reset();
		}

	public:

		Event()
			: isInitialized(false)
		{
		}

		Event(EventReset::Type resetType, bool initialState)
			: isInitialized(false)
		{
			Create(resetType, initialState);
		}

		~Event()
		{
			if (isInitialized)
			{
				pthread_cond_destroy( &condition );
				pthread_mutex_destroy( &mutex );
			}
		}

		void Create(EventReset::Type _resetType, bool initialState)
		{
			ASSERT (!isInitialized, "Event already initialized");

			resetType = _resetType;

			pthread_mutex_init( &mutex, nullptr );
			pthread_cond_init( &condition, nullptr );
			val.Set(initialState ? SIGNALED : NOT_SIGNALED);

			isInitialized = true;
		}

		void Signal()
		{
			ASSERT (isInitialized, "Event not initialized");

			val.Set(SIGNALED);
			pthread_cond_broadcast( &condition );
		}

		void Reset()
		{
			ASSERT (isInitialized, "Event not initialized");

			val.Set(NOT_SIGNALED);
		}

		bool Wait(uint32 milliseconds)
		{
			ASSERT (isInitialized, "Event not initialized");

			// early exit if event already signaled
			if ( val.Get() != NOT_SIGNALED )
			{
				AutoResetIfNeed();
				return true;
			}

			//convert milliseconds to posix time
			struct timeval tv;
			gettimeofday( &tv, nullptr );
			struct timespec tm;
			tm.tv_sec = milliseconds / 1000 + tv.tv_sec;
			tm.tv_nsec = ( milliseconds % 1000 ) * 1000000 + tv.tv_usec * 1000;

			pthread_mutex_lock( &mutex );

			int ret = 0;
			do
			{
				ret = pthread_cond_timedwait( &condition, &mutex, &tm );
			} while( ret == EINTR );

			sched_yield();

			pthread_mutex_unlock( &mutex );

			AutoResetIfNeed();
			return ret == 0;
		}

	};

}


