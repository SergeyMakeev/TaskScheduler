#pragma once

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

	T* buffer()
	{
		return (T*)(data);
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
		return buffer()[i];
	}

	T &operator[]( uint32 i )
	{
		ASSERT( i < size(), "bad index" );
		return buffer()[i];
	}

	T& add()
	{
		ASSERT(count < capacity, "Can't add element");
		return buffer()[count++];
	}

	size_t size() const
	{
		return count;
	}

	bool empty() const
	{
		return count > 0;
	}

	T* begin()
	{
		return buffer();
	}
};


template<class T>
class fixed_array
{
	T* data;
	size_t count;
public:
	fixed_array(void* memoryChunk, size_t instanceCount) : data((T*)memoryChunk), count(instanceCount) {}

	const T &operator[]( uint32 i ) const
	{
		ASSERT( i < size(), "bad index" );
		return data[i];
	}

	T &operator[]( uint32 i )
	{
		ASSERT( i < size(), "bad index" );
		return data[i];
	}

	size_t size() const
	{
		return count;
	}

	bool empty() const
	{
		return count > 0;
	}
};




