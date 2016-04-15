// The MIT License (MIT)
//
// 	Copyright (c) 2015 Sergey Makeev, Vadim Slyusarev
//
// 	Permission is hereby granted, free of charge, to any person obtaining a copy
// 	of this software and associated documentation files (the "Software"), to deal
// 	in the Software without restriction, including without limitation the rights
// 	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// 	copies of the Software, and to permit persons to whom the Software is
// 	furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
// 	all copies or substantial portions of the Software.
//
// 	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// 	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// 	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// 	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// 	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// 	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// 	THE SOFTWARE.

#pragma once

#ifndef __MT_EVENT__
#define __MT_EVENT__


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

		uint32 numOfWaitingThreads;

		pthread_mutex_t	mutex;
		pthread_cond_t	condition;
		AtomicInt32Base val;
		EventReset::Type resetType;
		bool isInitialized;

	private:

		void AutoResetIfNeed()
		{
			if (resetType == EventReset::MANUAL)
			{
				return;
			}
			Reset();
		}

	public:

		MT_NOCOPYABLE(Event);


		Event()
			: numOfWaitingThreads(0)
			, isInitialized(false)
		{
		}

		Event(EventReset::Type resetType, bool initialState)
			: numOfWaitingThreads(0)
			, isInitialized(false)
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
			MT_ASSERT (!isInitialized, "Event already initialized");

			resetType = _resetType;

			pthread_mutex_init( &mutex, nullptr );
			pthread_cond_init( &condition, nullptr );
			val.Store(initialState ? SIGNALED : NOT_SIGNALED);

			isInitialized = true;
			numOfWaitingThreads = 0;
		}

		void Signal()
		{
			MT_ASSERT (isInitialized, "Event not initialized");

			pthread_mutex_lock( &mutex );

			val.Store(SIGNALED);

			if (numOfWaitingThreads)
			{
				if (resetType == EventReset::MANUAL)
				{
					pthread_cond_broadcast( &condition );
				} else
				{
					pthread_cond_signal( &condition );
				}
			}

			pthread_mutex_unlock( &mutex );
		}

		void Reset()
		{
			MT_ASSERT (isInitialized, "Event not initialized");
			val.Store(NOT_SIGNALED);
		}

		bool Wait(uint32 milliseconds)
		{
			MT_ASSERT (isInitialized, "Event not initialized");

			pthread_mutex_lock( &mutex );

			// early exit if event already signaled
			if ( val.Load() != NOT_SIGNALED )
			{
				AutoResetIfNeed();
				pthread_mutex_unlock( &mutex );
				return true;
			}

			numOfWaitingThreads++;

			//convert milliseconds to posix time
			struct timeval tv;
			gettimeofday( &tv, nullptr );
			struct timespec tm;
			tm.tv_sec = tv.tv_sec + milliseconds / 1000;
			uint64 nanosec = (uint64)tv.tv_usec * 1000 + (uint64)milliseconds * 1000000;
			if (nanosec >= 1000000000)
			{
				tm.tv_sec += (long)(nanosec / 1000000000);
				nanosec = nanosec % 1000000000;
			}
			tm.tv_nsec = (long)nanosec;

			int ret = 0;
			while(true)
			{
				ret = pthread_cond_timedwait( &condition, &mutex, &tm );

				MT_ASSERT(ret == 0 || ret == ETIMEDOUT || ret == EINTR, "Unexpected return value");

				if (ret != EINTR)
				{
					break;
				}
			}

			numOfWaitingThreads--;

			AutoResetIfNeed();

			pthread_mutex_unlock( &mutex );

			//return true if val in SIGNALED state instead of ret == 0 ?
			return ret == 0;
		}

	};

}


#endif
