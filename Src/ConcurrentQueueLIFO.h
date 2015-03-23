#pragma once

#include <vector>

#include "Platform.h"
#include "Tools.h"


namespace MT
{

	//TODO: This is very naive implementation, rewrite to multiple producer, multiple consumer lock less queue
	template<typename T>
	class ConcurrentQueueLIFO
	{
		MT::Mutex mutex;
		std::vector<T> queue;

	public:

		ConcurrentQueueLIFO()
		{
			queue.reserve(256);
		}

		~ConcurrentQueueLIFO()
		{
		}

		void Push(const T & item)
		{
			MT::ScopedGuard guard(mutex);

			queue.push_back(item);
		}

		void PushRange(const T* itemArray, size_t count)
		{
			MT::ScopedGuard guard(mutex);

			for (size_t i = 0; i < count; ++i)
				queue.push_back(itemArray[i]);
		}

		bool TryPop(T & item)
		{
			MT::ScopedGuard guard(mutex);

			if (queue.empty())
			{
				return false;
			}
			item = queue.back();
			queue.pop_back();
			return true;
		}

		size_t PopAll(T * dstBuffer, size_t dstBufferSize)
		{
			MT::ScopedGuard guard(mutex);
			size_t elementsCount = Min(queue.size(), dstBufferSize);
			for (size_t i = 0; i < elementsCount; i++)
			{
				dstBuffer[i] = std::move(queue[i]);
			}
			queue.clear();
			return elementsCount;
		}

		bool IsEmpty()
		{
			MT::ScopedGuard guard(mutex);
			return queue.empty();
		}


	};
}