#pragma once

#include <ucontext.h>

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
		{
            fiberContext.uc_stack.ss_sp = nullptr;
		}

		~Fiber()
		{
            if (fiberContext.uc_stack.ss_sp)
            {
                free(fiberContext.uc_stack.ss_sp);
                fiberContext.uc_stack.ss_sp = nullptr;
            }
		}


		void CreateFromCurrentThread()
		{
			getcontext(&fiberContext);

			func = nullptr;
			funcData = nullptr;
		}


		void Create(size_t stackSize, TThreadEntryPoint entryPoint, void *userData)
		{
			func = entryPoint;
			funcData = userData;

            getcontext(&fiberContext);
            fiberContext.uc_link = nullptr;
            fiberContext.uc_stack.ss_sp = malloc(stackSize);
            fiberContext.uc_stack.ss_size = stackSize;
            fiberContext.uc_stack.ss_flags = 0;
            makecontext(&fiberContext, (void(*)())&FiberFuncInternal, 1, this);
		}


		void SwitchTo()
		{
			setcontext(&fiberContext);
		}


	};


}


