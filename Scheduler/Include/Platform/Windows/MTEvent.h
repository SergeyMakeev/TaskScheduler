#pragma once


namespace MT
{
	//
	//
	//
	class Event
	{
		::HANDLE eventHandle;

	private:

		Event(const Event&) {}
		void operator=(const Event&) {}

	public:

		Event()
		{
			static_assert(sizeof(Event) == sizeof(::HANDLE), "sizeof(Event) is invalid");
			eventHandle = nullptr;
		}

		Event(EventReset::Type resetType, bool initialState)
		{
			eventHandle = nullptr;
			Create(resetType, initialState);
		}

		~Event()
		{
			CloseHandle(eventHandle);
			eventHandle = nullptr;
		}

		void Create(EventReset::Type resetType, bool initialState)
		{
			if (eventHandle != nullptr)
			{
				CloseHandle(eventHandle);
			}

			BOOL bManualReset = (resetType == EventReset::AUTOMATIC) ? FALSE : TRUE;
			BOOL bInitialState = initialState ? TRUE : FALSE;
			eventHandle = ::CreateEvent(nullptr, bManualReset, bInitialState, nullptr);
		}

		void Signal()
		{
			SetEvent(eventHandle);
		}

		void Reset()
		{
			ResetEvent(eventHandle);
		}

		bool Wait(uint32 milliseconds)
		{
			DWORD res = WaitForSingleObject(eventHandle, milliseconds);
			return (res == WAIT_OBJECT_0);
		}

	};

}


