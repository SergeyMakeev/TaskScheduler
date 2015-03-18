#pragma once


namespace MT
{
	class ScopedGuard;

	//
	//
	//
	class Mutex
	{
		pthread_mutexattr_t mutexAttr;
		pthread_mutex_t mutex;

	private:

		Mutex(const Mutex &) {}
		void operator=(const Mutex &) {}

	public:

		Mutex()
		{
			int res = pthread_mutexattr_init(&mutexAttr);
			ASSERT(res == 0, "pthread_mutexattr_init - failed");

			res = pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
			ASSERT(res == 0, "pthread_mutexattr_settype - failed");

			res = pthread_mutex_init(&mutex, &mutexAttr);
			ASSERT(res == 0, "pthread_mutex_init - failed");
		}

		~Mutex()
		{
			int res = pthread_mutex_destroy(&mutex);
			ASSERT(res == 0, "pthread_mutex_destroy - failed");

			res = pthread_mutexattr_destroy(&mutexAttr);
			ASSERT(res == 0, "pthread_mutexattr_destroy - failed");
		}

		friend class MT::ScopedGuard;

	private:

		void Lock()
		{
			int res = pthread_mutex_lock(&mutex);
			ASSERT(res == 0, "pthread_mutex_lock - failed");
		}
		void Unlock()
		{
			int res = pthread_mutex_unlock(&mutex);
			ASSERT(res == 0, "pthread_mutex_unlock - failed");
		}

	};


}


