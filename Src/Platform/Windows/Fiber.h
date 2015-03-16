#pragma once


namespace MT
{

	//
	// 
	//
	class _Fiber
	{
		void * funcData;
		TThreadEntryPoint func;

		void* fiber;

		static void __stdcall FiberFuncInternal(void *pFiber)
		{
			_Fiber* self = (_Fiber*)pFiber;
			self->func(self->funcData);
		}

	private:

		_Fiber(const _Fiber &) {}
		void operator=(const _Fiber &) {}

	public:

		_Fiber()
			: fiber(nullptr)
		{
		}

		~_Fiber()
		{
			if (fiber)
			{
				DeleteFiber(fiber);
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


