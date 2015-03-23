#pragma once


template<class T>
class fixed_array
{
	T* data;
	size_t count;
public:
	fixed_array(void* memoryChunk, size_t instanceCount)
		: data((T*)memoryChunk)
		, count(instanceCount)
	{
		ASSERT(data, "Invalid data array");
	}

	const T &operator[]( size_t i ) const
	{
		ASSERT( i < size(), "bad index" );
		return data[i];
	}

	T &operator[]( size_t i )
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




