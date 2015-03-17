#pragma once


namespace MT
{
	class ScopedGuard;

	//
	//
	//
	class Mutex
	{
		pthread_mutex_t mutex;

	private:

		Mutex(const Mutex &) {}
		void operator=(const Mutex &) {}

	public:

		Mutex()
		{
			int res = pthread_mutex_init(&mutex, nullptr);
			ASSERT(res == 0, "pthread_mutex_init - failed");
		}

		~Mutex()
		{
			int res = pthread_mutex_destroy(&mutex);
			ASSERT(res == 0, "pthread_mutex_destroy - failed");
		}

		friend class MT::ScopedGuard;

	private:

		void Lock()
		{
			pthread_mutex_lock(&mutex);
		}
		void Unlock()
		{
			pthread_mutex_unlock(&mutex);
		}

	};


}


