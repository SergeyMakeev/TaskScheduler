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

#include <MTTools.h>
#include <MTPlatform.h>
#include <MTConcurrentQueueLIFO.h>
#include <MTStackArray.h>
#include <MTArrayView.h>


namespace MT
{
	class FiberContext;

	namespace internal
	{
		struct ThreadContext;
	}

#define ALIVE_STAMP (0x33445566)

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Task group
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Application can assign task group to task and later wait until group was finished.
	class TaskGroup
	{
		AtomicInt inProgressTaskCount;
		Event allDoneEvent;

		//Tasks awaiting group through FiberContext::WaitGroupAndYield call
		ConcurrentQueueLIFO<FiberContext*> waitTasksQueue;

		uint32 aliveStamp;

	public:

		TaskGroup()
		{
			aliveStamp = 0x33445566;

			inProgressTaskCount.Set(0);
			allDoneEvent.Create( EventReset::MANUAL, true );
		}

		~TaskGroup()
		{
			aliveStamp = 0xFDFDFDFD;
		}

		int GetTaskCount() const
		{
			MT_ASSERT(this != nullptr, "AV exception here");
			MT_ASSERT(aliveStamp == ALIVE_STAMP, "Group already destroyed");

			return inProgressTaskCount.Get();
		}

		ConcurrentQueueLIFO<FiberContext*> & GetWaitQueue()
		{
			MT_ASSERT(this != nullptr, "AV exception here");
			MT_ASSERT(aliveStamp == ALIVE_STAMP, "Group already destroyed");

			return waitTasksQueue;
		}

		int Dec()
		{
			MT_ASSERT(this != nullptr, "AV exception here");
			MT_ASSERT(aliveStamp == ALIVE_STAMP, "Group already destroyed");

			return inProgressTaskCount.Dec();
		}

		int Inc()
		{
			MT_ASSERT(this != nullptr, "AV exception here");
			MT_ASSERT(aliveStamp == ALIVE_STAMP, "Group already destroyed");

			return inProgressTaskCount.Inc();
		}

		int Add(int sum)
		{
			MT_ASSERT(this != nullptr, "AV exception here");
			MT_ASSERT(aliveStamp == ALIVE_STAMP, "Group already destroyed");

			return inProgressTaskCount.Add(sum);
		}

		void Signal()
		{
			MT_ASSERT(this != nullptr, "AV exception here");
			MT_ASSERT(aliveStamp == ALIVE_STAMP, "Group already destroyed");

			allDoneEvent.Signal();
		}

		void Reset()
		{
			MT_ASSERT(this != nullptr, "AV exception here");
			MT_ASSERT(aliveStamp == ALIVE_STAMP, "Group already destroyed");

			allDoneEvent.Reset();
		}

		bool Wait(uint32 milliseconds)
		{
			MT_ASSERT(this != nullptr, "AV exception here");
			MT_ASSERT(aliveStamp == ALIVE_STAMP, "Group already destroyed");

			return allDoneEvent.Wait(milliseconds);
		}


		
	};



}
