#pragma once

#define ALLOCATE_ON_STACK(TYPE, COUNT) (TYPE*)_alloca(sizeof(TYPE) * COUNT)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Array, allocated on stack
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T, size_t capacity>
class stack_array
{
	char data[sizeof(T) * capacity];
	size_t count;

	stack_array(stack_array& other)
	{
		ASSERT(false, "Can't copy stack_vector");
	}

	stack_array& operator=(const stack_array&)
	{
		ASSERT(false, "Can't copy stack_vector");
	}

public:
	inline stack_array(size_t _count = 0) : count(_count)
	{
		ASSERT(count <= capacity, "Too big size")
	}

	bool resize(size_t instanceCount)
	{
		VERIFY(instanceCount < capacity, "Can't resize, too big size", return false);
		count = instanceCount;
		return true;
	}

	const T &operator[]( uint32 i ) const
	{
		ASSERT( i < size(), "bad index" );
		return GetBuffer()[i];
	}

	T &operator[]( uint32 i )
	{
		ASSERT( i < size(), "bad index" );
		return GetBuffer()[i];
	}

	T& add()
	{
		ASSERT(count < capacity, "Can't add element");
		return GetBuffer()[count++];
	}

	size_t size() const
	{
		return count;
	}

	bool empty() const
	{
		return count > 0;
	}

	T* GetBuffer()
	{
		return (T*)(data);
	}
};



