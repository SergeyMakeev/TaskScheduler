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

#include <ucontext.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

namespace MT
{

	//
	//
	//
	class Fiber
	{
		void * funcData;
		TThreadEntryPoint func;

		char* stackRawMemory;
		size_t stackRawMemorySize;

		ucontext_t fiberContext;
		bool isInitialized;

		static void FiberFuncInternal(void *pFiber)
		{
			MT_ASSERT(pFiber != nullptr, "Invalid fiber");
			Fiber* self = (Fiber*)pFiber;

			MT_ASSERT(self->isInitialized == true, "Using non initialized fiber");

			MT_ASSERT(self->func != nullptr, "Invalid fiber func");
			self->func(self->funcData);
		}

	private:

		Fiber(const Fiber &) {}
		void operator=(const Fiber &) {}

	public:

		Fiber()
			: funcData(nullptr)
			, func(nullptr)
			, stackRawMemory(nullptr)
			, stackRawMemorySize(0)
			, isInitialized(false)
		{
			memset(&fiberContext, 0, sizeof(ucontext_t));
		}

		~Fiber()
		{
			if (isInitialized)
			{
				// if func != null than we have memory ownership
				if (func != nullptr)
				{
					int res = munmap(stackRawMemory, stackRawMemorySize);
					MT_ASSERT(res == 0, "Can't free memory");
				}

				isInitialized = false;
			}
		}


		void CreateFromThread(Thread & thread)
		{
			MT_ASSERT(!isInitialized, "Already initialized");
			MT_ASSERT(thread.IsCurrentThread(), "ERROR: Can create fiber only from current thread!");

			int res = getcontext(&fiberContext);
			MT_ASSERT(res == 0, "getcontext - failed");

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
			MT_ASSERT(!isInitialized, "Already initialized");

			func = entryPoint;
			funcData = userData;

			int res = getcontext(&fiberContext);
			MT_ASSERT(res == 0, "getcontext - failed");


			int pageSize = sysconf(_SC_PAGE_SIZE);
			int pagesCount = stackSize / pageSize;

			//need additional page for stack tail
			if ((stackSize % pageSize) > 0)
			{
				pagesCount++;
			}

			//protected guard page
			pagesCount++;

			stackRawMemorySize = pagesCount * pageSize;

			stackRawMemory = (char*)mmap(NULL, stackRawMemorySize, PROT_READ | PROT_WRITE,  MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);

			MT_ASSERT((void *)stackRawMemory != (void *)-1, "Can't allocate memory");

			char* stackBottom = stackRawMemory + pageSize;
			//char* stackTop = stackRawMemory + stackMemorySize;

			res = mprotect(stackRawMemory, pageSize, PROT_NONE);
			MT_ASSERT(res == 0, "Can't protect memory");

			fiberContext.uc_link = nullptr;
			fiberContext.uc_stack.ss_sp = stackBottom;
			fiberContext.uc_stack.ss_size = stackRawMemorySize - pageSize;
			fiberContext.uc_stack.ss_flags = 0;

			makecontext(&fiberContext, (void(*)())&FiberFuncInternal, 1, (void *)this);

			isInitialized = true;
		}

		static void SwitchTo(Fiber & from, Fiber & to)
		{
			 __sync_synchronize();

			MT_ASSERT(from.isInitialized, "Invalid from fiber");
			MT_ASSERT(to.isInitialized, "Invalid to fiber");

			int res = swapcontext(&from.fiberContext, &to.fiberContext);
			MT_ASSERT(res == 0, "setcontext - failed");
		}



	};


}


