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

#define MT_FIBER_DEBUG (1)

namespace MT
{

	//
	// 
	//
	class Fiber
	{
		void * funcData;
		TThreadEntryPoint func;

		void* fiber;

#if MT_FIBER_DEBUG
		AtomicInt counter;
		AtomicInt ownerThread;
#endif

		static void __stdcall FiberFuncInternal(void *pFiber)
		{
			Fiber* self = (Fiber*)pFiber;
			self->func(self->funcData);
		}

	private:

		Fiber(const Fiber &) {}
		void operator=(const Fiber &) {}

	public:

		Fiber()
			: fiber(nullptr)
		{
		}

		~Fiber()
		{
			if (fiber)
			{
				//we don't need to delete context on fibers created from thread.
				if (func != nullptr)
				{
					DeleteFiber(fiber);
				}

				fiber = nullptr;
			}
		}


		void CreateFromThread(Thread & thread)
		{
			MT_ASSERT(fiber == nullptr, "Fiber already created");
			MT_ASSERT(thread.IsCurrentThread(), "Can't create fiber from this thread");

			func = nullptr;
			funcData = nullptr;

			fiber = ::ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
			MT_ASSERT(fiber != nullptr, "Can't create fiber");

#if MT_FIBER_DEBUG
			ownerThread.Set(0xFFFFFFFF);
			counter.Set(1);
#endif
		}


		void Create(size_t stackSize, TThreadEntryPoint entryPoint, void *userData)
		{
			MT_ASSERT(fiber == nullptr, "Fiber already created");

			func = entryPoint;
			funcData = userData;
			fiber = ::CreateFiber( stackSize, FiberFuncInternal, this );
			MT_ASSERT(fiber != nullptr, "Can't create fiber");

#if MT_FIBER_DEBUG
			ownerThread.Set(0xFFFFFFFF);
			counter.Set(0);
#endif
		}

#if MT_FIBER_DEBUG
		int GetUsageCounter() const
		{
			return counter.Get();
		}

		int GetOwnerThread() const
		{
			return ownerThread.Get();
		}

#endif


		static void SwitchTo(Fiber & from, Fiber & to)
		{
			MemoryBarrier();

			MT_ASSERT(from.fiber != nullptr, "Invalid from fiber");
			MT_ASSERT(to.fiber != nullptr, "Invalid to fiber");

#if MT_FIBER_DEBUG

			to.ownerThread.Set(::GetCurrentThreadId());

			MT_ASSERT(from.counter.Get() == 1, "Invalid fiber state");
			MT_ASSERT(to.counter.Get() == 0, "Invalid fiber state");

			int counterNow = from.counter.Dec();
			MT_ASSERT(counterNow == 0, "Invalid fiber state");

			counterNow = to.counter.Inc();
			MT_ASSERT(counterNow == 1, "Invalid fiber state");
#endif

			::SwitchToFiber( (LPVOID)to.fiber );
		}


	};


}


