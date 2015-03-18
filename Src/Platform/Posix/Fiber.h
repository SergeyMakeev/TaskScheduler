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


		void CreateFromThread(Thread & thread)
		{
			ASSERT(!isInitialized, "Already initialized");
			ASSERT(thread.IsCurrentThread(), "Can't create fiber from this thread");

			int res = getcontext(&fiberContext);
			ASSERT(res == 0, "getcontext - failed");

			fiberContext.uc_link = nullptr;
			fiberContext.uc_stack.ss_sp = thread.GetStackBase();
			fiberContext.uc_stack.ss_size = thread.GetStackSize();
			fiberContext.uc_stack.ss_flags = 0;

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

		static void SwitchTo(Fiber & from, Fiber & to)
		{
			ASSERT(from.isInitialized != nullptr, "Invalid from fiber");
			ASSERT(to.isInitialized != nullptr, "Invalid to fiber");
			int res = swapcontext(&from.fiberContext, &to.fiberContext);
			ASSERT(res == 0, "setcontext - failed");
		}



	};


}


