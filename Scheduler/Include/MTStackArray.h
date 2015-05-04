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


namespace MT
{

	/// \class StackArray
	/// \brief Array, allocated on stack.
	template<class T, size_t capacity>
	class StackArray
	{
		char data[sizeof(T) * capacity];
		size_t count;

		inline T* Buffer()
		{
			return (T*)(data);
		}

		inline void CopyCtor(T* element, const T & val)
		{
			new(element) T(val);
		}

		inline void MoveCtor(T* element, const T && val)
		{
			new(element) T(std::move(val));
		}

		inline void Dtor(T* element)
		{
#if _MSC_VER
			// warning C4100: 'element' : unreferenced formal parameter
			// if type T has not destructor
			element;
#endif
			element->~T();
		}

	private:

		StackArray(StackArray& ) {}
		void operator=(const StackArray&) {}

	public:

		inline StackArray()
			: count(0)
		{
		}

		inline StackArray(size_t _count, const T & defaultElement = T())
			: count(_count)
		{
			ASSERT(count <= capacity, "Too big size");
			for (size_t i = 0; i < count; i++)
			{
				CopyCtor(Begin() + i, defaultElement);
			}
		}

		inline ~StackArray()
		{
			for (size_t i = 0; i < count; i++)
			{
				Dtor(Begin() + i);
			}
		}

		inline const T &operator[]( uint32 i ) const
		{
			ASSERT( i < Size(), "bad index" );
			return Buffer()[i];
		}

		inline T &operator[]( uint32 i )
		{
			ASSERT( i < Size(), "bad index" );
			return Buffer()[i];
		}

		inline void PushBack(const T && val)
		{
			ASSERT(count < capacity, "Can't add element");
			size_t lastElementIndex = count;
			count++;
			MoveCtor( Buffer() + lastElementIndex, std::move(val) );
		}

		inline size_t Size() const
		{
			return count;
		}

		inline bool IsEmpty() const
		{
			return count == 0;
		}

		inline T* Begin()
		{
			return Buffer();
		}
	};



}

