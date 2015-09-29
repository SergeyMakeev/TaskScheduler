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

#ifdef _WIN32
#include <crtdefs.h>
#else
#include <sys/types.h>
#include <stddef.h>
#endif

#define MT_DEFAULT_ALIGN (16)

namespace MT
{
	// Memory allocator interface.
	//////////////////////////////////////////////////////////////////////////
	struct Memory
	{
		struct StackDesc
		{
			void * stackBottom;
			void * stackTop;

			char * stackMemory;
			size_t stackMemoryBytesCount;

			StackDesc()
				: stackBottom(nullptr)
				, stackTop(nullptr)
				, stackMemory(nullptr)
				, stackMemoryBytesCount(0)
			{
			}

			size_t GetStackSize()
			{
				return (char*)stackTop - (char*)stackBottom;
			}
		};


		static void* Alloc(size_t size, size_t align = MT_DEFAULT_ALIGN);
		static void Free(void* p);

		static StackDesc AllocStack(size_t size);
		static void FreeStack(const StackDesc & desc);
	};


	/// \class Allocator
	/// \brief Redirecting allocator from std to Custom memory allocator
	///        Based on 'Mallocator' source code http://blogs.msdn.com/b/vcblog/archive/2008/08/28/the-mallocator.aspx
	//////////////////////////////////////////////////////////////////////////
	template <typename T>
	class StdAllocator
	{
	public:

		// The following will be the same for virtually all allocators.
		typedef T * pointer;
		typedef const T * const_pointer;
		typedef T& reference;
		typedef const T& const_reference;
		typedef T value_type;
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;

		T * address(T& r) const
		{
			return &r;
		}

		const T * address(const T& s) const
		{
			return &s;
		}

		size_t max_size() const
		{
			// The following has been carefully written to be independent of
			// the definition of size_t and to avoid signed/unsigned warnings.
			return (static_cast<size_t>(0) - static_cast<size_t>(1)) / sizeof(T);
		}

		// The following must be the same for all allocators.
		template <typename U>
		struct rebind
		{
			typedef StdAllocator<U> other;
		};

		bool operator!=(const StdAllocator& other) const
		{
			return !(*this == other);
		}

		void construct(T * const p, const T& t) const
		{
			void * const pv = static_cast<void *>(p);
			new (pv) T(t);
		}

		void destroy(T * const p) const
		{
#if defined(_MSC_VER)
			p;
#endif
			p->~T();
		}

		// Returns true if and only if storage allocated from *this
		// can be deallocated from other, and vice versa.
		bool operator==(const StdAllocator& other) const
		{
			return true;
		}

		// Default constructor, copy constructor, rebinding constructor, and destructor.
		StdAllocator() {}
		StdAllocator(const StdAllocator&) {}
		template <typename U> StdAllocator(const StdAllocator<U>&) {}
		~StdAllocator() {}

		// The following will be different for each allocator.
		T * allocate(const size_t n) const
		{
			// The return value of allocate(0) is unspecified.
			// Mallocator returns NULL in order to avoid depending
			// on malloc(0)'s implementation-defined behavior
			// (the implementation can define malloc(0) to return NULL,
			// in which case the bad_alloc check below would fire).
			// All allocators can return NULL in this case.
			if (n == 0)
				return nullptr;

			// All allocators should contain an integer overflow check.
			// The Standardization Committee recommends that std::length_error
			// be thrown in the case of integer overflow.
			if (n >= max_size())
			{
				//MT_ASSERT(false, "StdAllocator<T>::allocate() - Integer overflow.");
				return nullptr;
			}


			// Memory allocate
			void * const pv = Memory::Alloc(n * sizeof(T));

			// Allocators should return NULL in the case of memory allocation failure.
			if (pv == nullptr)
			{
				//MT_ASSERT(false, "Bad alloc!");
				return nullptr;
			}

			return static_cast<T *>(pv);
		}

		void deallocate(T * const p, const size_t) const
		{
			Memory::Free(p);
		}

		// The following will be the same for all allocators that ignore hints.
		template <typename U>
		T * allocate(const size_t n, const U * /* const hint */) const
		{
			return allocate(n);
		}

		// Allocators are not required to be assignable, so
		// all allocators should have a private unimplemented
		// assignment operator. Note that this will trigger the
		// off-by-default (enabled under /Wall) warning C4626
		// "assignment operator could not be generated because a
		// base class assignment operator is inaccessible" within
		// the STL headers, but that warning is useless.
	private:
		StdAllocator& operator=(const StdAllocator&);
	};



}
