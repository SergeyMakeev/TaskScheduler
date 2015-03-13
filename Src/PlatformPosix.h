#pragma once

//Posix system

#include <pthread.h>

namespace MT
{
    typedef pthread_t Thread;

	inline MT::Thread CreateSuspendedThread(size_t stackSize, TThreadStartFunc entryPoint, void* userData)
	{
		LPTHREAD_START_ROUTINE pEntryPoint = (LPTHREAD_START_ROUTINE)entryPoint;

		int res = 0;

        pthread_attr_t threadAttr;
        res = pthread_attr_init(&threadAttr);
        ASSERT(res == 0, "Create thread error");

        res = pthread_attr_setstacksize(&threadAttr, stackSize);
        ASSERT(res == 0, "Create thread error");

		pthread_t thread;

        res = pthread_create(&thread, &threadAttr, pEntryPoint, userData);
        ASSERT(res == 0, "Create thread error");

		return thread;
	}


	//
	// Atomic int
	//
	class AtomicInt
	{
		volatile long value;
	public:

		AtomicInt() : value(0)
		{
		}

		AtomicInt(int v)
			: value(v)
		{
		}

		void Add(int sum)
		{
			__sync_add_and_fetch(&value, sum);
		}

		int Inc()
		{
			return __sync_add_and_fetch(&value, 1);
		}

		int Dec()
		{
			return __sync_sub_and_fetch(&value, 1);
		}

		int Get()
		{
			return value;
		}

		int Set(int val)
		{
			return InterlockedExchange(&value, val); 
		}
	};



}

