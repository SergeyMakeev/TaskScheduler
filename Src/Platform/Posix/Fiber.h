#pragma once

#include <ucontext.h>
#include <stdlib.h>

namespace MT
{

	//
	//
	//
	class Fiber
	{
		void * funcData;
		TThreadEntryPoint func;

		ucontext_t fiberContext;
		bool isInitialized;

		static void FiberFuncInternal(void *pFiber)
		{
			Fiber* self = (Fiber*)pFiber;
			self->func(self->funcData);
		}

	private:

		Fiber(const Fiber &) {}
		void operator=(const Fiber &) {}

	public:

		Fiber()
			: isInitialized(false)
		{
		}

		~Fiber()
		{
			if (isInitialized)
			{
				if (func != nullptr)
				{
					free(fiberContext.uc_stack.ss_sp);
				}
				isInitialized = false;
			}
		}


		void CreateFromCurrentThread()
		{
			ASSERT(!isInitialized, "Already initialized");

			int res = getcontext(&fiberContext);
			ASSERT(res == 0, "getcontext - failed");

			func = nullptr;
			funcData = nullptr;

			isInitialized = true;
		}


		void Create(size_t stackSize, TThreadEntryPoint entryPoint, void *userData)
		{
			ASSERT(!isInitialized, "Already initialized");

			func = entryPoint;
			funcData = userData;

			int res = getcontext(&fiberContext);
			ASSERT(res == 0, "getcontext - failed");

			fiberContext.uc_link = nullptr;
			fiberContext.uc_stack.ss_sp = malloc(stackSize);
			fiberContext.uc_stack.ss_size = stackSize;
			fiberContext.uc_stack.ss_flags = 0;
			makecontext(&fiberContext, (void(*)())&FiberFuncInternal, 1, this);

			isInitialized = true;
		}


		void SwitchTo()
		{
			ASSERT(isInitialized, "Can't use uninitialized fiber");

			int res = setcontext(&fiberContext);
			ASSERT(res == 0, "setcontext - failed");
		}


	};


}


