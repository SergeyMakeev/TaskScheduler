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

#include <Platform/Common/MTThread.h>

namespace MT
{
	class _Fiber;

	class Thread : public ThreadBase
	{
		DWORD threadId;
		HANDLE thread;

		static DWORD __stdcall ThreadFuncInternal(void *pThread)
		{
			Thread * self = (Thread *)pThread;

			self->func(self->funcData);

			ExitThread(0);
		}
	public:

		Thread()
			: thread(nullptr)
			, threadId(0)
		{
		}

		~Thread()
		{
			ASSERT(thread == nullptr, "Thread is not stopped!")
		}

		void Start(size_t stackSize, TThreadEntryPoint entryPoint, void *userData)
		{
			ASSERT(thread == nullptr, "Thread already started");

			func = entryPoint;
			funcData = userData;
			thread = ::CreateThread( nullptr, stackSize, ThreadFuncInternal, this, 0, &threadId );
			ASSERT(thread != nullptr, "Can't create thread");
		}

		void Stop()
		{
			if (thread == nullptr)
			{
				return;
			}

			::WaitForSingleObject(thread, INFINITE);
			BOOL res = CloseHandle(thread);
			ASSERT(res != 0, "Can't close thread handle")
			thread = nullptr;
		}

		bool IsCurrentThread() const
		{
			return threadId == ::GetCurrentThreadId();
		}

		static int GetNumberOfHardwareThreads()
		{
			SYSTEM_INFO sysinfo;
			::GetSystemInfo( &sysinfo );
			return sysinfo.dwNumberOfProcessors;
		}

		static void Sleep(uint32 milliseconds)
		{
		  ::Sleep(milliseconds);
		}
	};


}


