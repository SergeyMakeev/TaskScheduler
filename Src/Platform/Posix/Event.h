#pragma once

#include <sched.h>

namespace MT
{
	//
	//
	//
	class Event
	{
		pthread_mutex_t	mutex;
		pthread_cond_t	condition;
		AtomicInt val;
		bool isInitialized;

	private:

		Event(const Event&) {}
		void operator=(const Event&) {}

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

		void Create(EventReset::Type resetType, bool initialState)
		{
			ASSERT (!isInitialized, "Event already initialized");

			pthread_mutex_init( &mutex, nullptr );
			pthread_cond_init( &condition, nullptr );
			val.Set(0);
		}

		void Signal()
		{
			ASSERT (isInitialized, "Event not initialized");

			val.Set(1);
			pthread_cond_broadcast( &condition );
		}

		void Reset()
		{
			ASSERT (isInitialized, "Event not initialized");

			val.Set(0);
		}

		bool Wait(uint32 milliseconds)
		{
			ASSERT (isInitialized, "Event not initialized");

			if ( val.Get() != 0 )
			{
				val.Set(0);
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
			return ret == 0;
		}

	};

}


