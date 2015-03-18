#pragma once


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
		}


		void Create(size_t stackSize, TThreadEntryPoint entryPoint, void *userData)
		{
			ASSERT(fiber == nullptr, "Fiber already created");

			func = entryPoint;
			funcData = userData;
			fiber = ::CreateFiber( stackSize, FiberFuncInternal, this );
			ASSERT(fiber != nullptr, "Can't create fiber");
		}

		static void SwitchTo(Fiber & from, Fiber & to)
		{
			ASSERT(from.fiber != nullptr, "Invalid from fiber");
			ASSERT(to.fiber != nullptr, "Invalid to fiber");
			::SwitchToFiber( (LPVOID)to.fiber );
		}

	};


}


