#pragma once

// Array, allocated on stack
template<class T, size_t capacity>
class stack_array
{
	char data[sizeof(T) * capacity];
	size_t count;

	stack_array(stack_array& other) {}
	void operator=(const stack_array&) {}

	inline T* buffer()
	{
		return (T*)(data);
	}

	inline void copy_ctor(T* element, const T & val)
	{
		new(element) T(val);
	}

	inline void move_ctor(T* element, const T && val)
	{
		new(element) T(std::move(val));
	}

	inline void dtor(T* element)
	{
		element;
		element->~T();
	}

public:


	inline stack_array() : count(0)
	{
	}

	inline stack_array(size_t _count, const T & defaultElement = T()) : count(_count)
	{
		ASSERT(count <= capacity, "Too big size");
		for (size_t i = 0; i < count; i++)
		{
			copy_ctor(begin() + i, defaultElement);
		}
	}

	inline ~stack_array()
	{
		for (size_t i = 0; i < count; i++)
		{
			dtor(begin() + i);
		}
	}

	inline const T &operator[]( uint32 i ) const
	{
		ASSERT( i < size(), "bad index" );
		return buffer()[i];
	}

	inline T &operator[]( uint32 i )
	{
		ASSERT( i < size(), "bad index" );
		return buffer()[i];
	}

	inline void push_back(const T && val)
	{
		ASSERT(count < capacity, "Can't add element");
		size_t lastElementIndex = count;
		count++;
		move_ctor( buffer() + lastElementIndex, std::move(val) );
	}

	inline size_t size() const
	{
		return count;
	}

	inline bool empty() const
	{
		return count > 0;
	}

	inline T* begin()
	{
		return buffer();
	}
};



