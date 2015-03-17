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


		void CreateFromCurrentThread()
		{
			ASSERT(fiber == nullptr, "Fiber already created");

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


		void SwitchTo()
		{
			ASSERT(fiber != nullptr, "Invalid fiber");
			::SwitchToFiber( (LPVOID)fiber );
		}
		

	};


}


