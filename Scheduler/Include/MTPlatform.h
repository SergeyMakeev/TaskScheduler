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

#include <MTTypes.h>
#include <MTDebug.h>

typedef void (*TThreadEntryPoint)(void* userData);

#define MT_ARRAY_SIZE( arr ) ( sizeof( arr ) / sizeof( (arr)[0] ) )

namespace MT
{
	namespace EventReset
	{
		enum Type
		{
			AUTOMATIC = 0,
			MANUAL = 1,
		};
	}
}


inline bool IsPointerAligned( const volatile void* p, const uint32 align )
{
	return !((uintptr_t)p & (align - 1));
}

#ifdef _WIN32
	#include <Platform/Windows/MTCommon.h>
#else
	#include <Platform/Posix/MTCommon.h>
#endif


namespace MT
{
	//compile time POD type check
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	static_assert(std::is_pod<AtomicInt32Base>::value == true, "AtomicInt32Base must be a POD (plain old data type)");


	//
	// Atomic int (type with constructor)
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class AtomicInt32 : public AtomicInt32Base
	{

	public:

		AtomicInt32()
		{
			_value = 0;
			static_assert(sizeof(AtomicInt32) == 4, "Invalid type size");
			MT_ASSERT(IsPointerAligned(&_value, 4), "Invalid atomic int alignment");
		}

		explicit AtomicInt32(int v)
		{
			_value = v;
			static_assert(sizeof(AtomicInt32) == 4, "Invalid type size");
			MT_ASSERT(IsPointerAligned(&_value, 4), "Invalid atomic int alignment");
		}
	};


	//compile time POD type check
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	static_assert(std::is_pod<AtomicPtrBase>::value == true, "AtomicPtrBase must be a POD (plain old data type)");


	//
	// Atomic pointer (type with constructor)
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	class AtomicPtr : protected AtomicPtrBase
	{
	public:
		AtomicPtr()			
		{
			_value = nullptr;
			MT_ASSERT(IsPointerAligned(&_value, sizeof(void*)), "Invalid atomic ptr alignment");
		}

		explicit AtomicPtr(T* v)
		{
			_value = v;
			MT_ASSERT(IsPointerAligned(&_value, sizeof(void*)), "Invalid atomic ptr alignment");
		}

		T* Load() const
		{
			return (T*)AtomicPtrBase::Load();
		}

		T* Store(T* val)
		{
			return (T*)AtomicPtrBase::Store(val);
		}


		T* CompareAndSwap(T* compareValue, T* newValue)
		{
			return (T*)AtomicPtrBase::CompareAndSwap(compareValue, newValue);
		}

		T* LoadRelaxed() const
		{
			return (T*)AtomicPtrBase::LoadRelaxed();
		}

		void StoreRelaxed(T* val)
		{
			AtomicPtrBase::StoreRelaxed(val);
		}
	};




	//
	//
	//
	class ScopedGuard
	{
		MT::Mutex & mutex;

		ScopedGuard( const ScopedGuard & ) : mutex(*((MT::Mutex*)nullptr)) {}
		void operator=( const ScopedGuard &) {}

	public:

		ScopedGuard(MT::Mutex & _mutex) : mutex(_mutex)
		{
			mutex.Lock();
		}

		~ScopedGuard()
		{
			mutex.Unlock();
		}
	};

	//
	// Simple Linear congruential generator
	//
	class LcgRandom
	{
		uint32 state;

	public:

		LcgRandom()
			: state(2578432553)
		{
		}

		void SetSeed(uint32 seed)
		{
			state = seed;
		}

		uint16 Get()
		{
			state = 214013 * state + 2531011;
			uint16 rnd = (state >> 16);
			return rnd;
		}


	};

}

