#pragma once


namespace MT
{
	//
	//
	//
	class Event
	{
		//::HANDLE eventHandle;

	private:

		Event(const Event&) {}
		void operator=(const Event&) {}

	public:

		Event()
		{
			//static_assert(sizeof(Event) == sizeof(::HANDLE), "sizeof(Event) is invalid");
			//eventHandle = NULL;
		}

		Event(EventReset::Type resetType, bool initialState)
		{
			//eventHandle = NULL;
			//Create(resetType, initialState);
		}

		~Event()
		{
			//CloseHandle(eventHandle);
			//eventHandle = NULL;
		}

		void Create(EventReset::Type resetType, bool initialState)
		{
/*
			if (eventHandle != NULL)
			{
				CloseHandle(eventHandle);
			}

			BOOL bManualReset = (resetType == EventReset::AUTOMATIC) ? FALSE : TRUE;
			BOOL bInitialState = initialState ? TRUE : FALSE;
			eventHandle = ::CreateEvent(NULL, bManualReset, bInitialState, NULL);
*/
		}

		void Signal()
		{
			//SetEvent(eventHandle);
		}

		void Reset()
		{
			//ResetEvent(eventHandle);
		}

		bool Wait(uint32 milliseconds)
		{
/*
			DWORD res = WaitForSingleObject(eventHandle, milliseconds);
			return (res == WAIT_OBJECT_0);
*/
            return false;
		}

		bool Check()
		{
			return Wait(0);
		}

		static inline bool WaitAll(Event * eventsArray, uint32 size, uint32 milliseconds)
		{
			//DWORD res = WaitForMultipleObjects(size, (::HANDLE *)&eventsArray[0], TRUE, milliseconds);
			//return (res == WAIT_OBJECT_0);
			return false;
		}
	};

}


