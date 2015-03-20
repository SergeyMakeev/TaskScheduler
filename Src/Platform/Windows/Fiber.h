#pragma once


#define FIBER_DEBUG (1)

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

#if FIBER_DEBUG
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
			ASSERT(fiber == nullptr, "Fiber already created");
			ASSERT(thread.IsCurrentThread(), "Can't create fiber from this thread");

			func = nullptr;
			funcData = nullptr;

			fiber = ::ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
			ASSERT(fiber != nullptr, "Can't create fiber");

#if FIBER_DEBUG
			ownerThread.Set(0xFFFFFFFF);
			counter.Set(1);
#endif
		}


		void Create(size_t stackSize, TThreadEntryPoint entryPoint, void *userData)
		{
			ASSERT(fiber == nullptr, "Fiber already created");

			func = entryPoint;
			funcData = userData;
			fiber = ::CreateFiber( stackSize, FiberFuncInternal, this );
			ASSERT(fiber != nullptr, "Can't create fiber");

#if FIBER_DEBUG
			ownerThread.Set(0xFFFFFFFF);
			counter.Set(0);
#endif
		}

#if FIBER_DEBUG
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

			ASSERT(from.fiber != nullptr, "Invalid from fiber");
			ASSERT(to.fiber != nullptr, "Invalid to fiber");

#if FIBER_DEBUG

			to.ownerThread.Set(::GetCurrentThreadId());

			ASSERT(from.counter.Get() == 1, "Invalid fiber state");
			ASSERT(to.counter.Get() == 0, "Invalid fiber state");

			int counterNow = from.counter.Dec();
			ASSERT(counterNow == 0, "Invalid fiber state");

			counterNow = to.counter.Inc();
			ASSERT(counterNow == 1, "Invalid fiber state");
#endif

			::SwitchToFiber( (LPVOID)to.fiber );
		}


	};


}


