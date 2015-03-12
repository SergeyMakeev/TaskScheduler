#include "Platform.h"
#include <vector>

namespace MT
{

	//TODO: This is very naive implementation, rewrite to multiple producer, multiple consumer lockless queue
	template<typename T>
	class ConcurrentQueueLIFO
	{
		MT::CriticalSection criticalSection;
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
			MT::ScopedGuard guard(criticalSection);

			queue.push_back(item);
		}

		bool TryPop(T & item)
		{
			MT::ScopedGuard guard(criticalSection);

			if (queue.empty())
			{
				return false;
			}
			item = queue.back();
			queue.pop_back();
			return true;
		}


	};
}