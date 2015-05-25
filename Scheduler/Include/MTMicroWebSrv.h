// The MIT License (MIT)
// 
// 	Copyright (c) 2015 Sergey Makeev, Vadim Slyusarev
// 
// 	Permission is hereby granted, free of charge, to any person obtaining a copy
// 	of this software and associated documentation files (the "Software"), to deal
// 	in the Software without restriction, including without limitation the rights
// 	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// 	copies of the Software, and to permit persons to whom the Software is
// 	furnished to do so, subject to the following conditions:
// 
//  The above copyright notice and this permission notice shall be included in
// 	all copies or substantial portions of the Software.
// 
// 	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// 	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// 	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// 	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// 	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// 	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// 	THE SOFTWARE.

#pragma once

#ifdef MT_INSTRUMENTED_BUILD

#include <MTTools.h>
#include <MTPlatform.h>
#include <MTTypes.h>
#include <MTDebug.h>

#ifdef _WIN32

#include <basetsd.h>
typedef UINT_PTR TcpSocket;

#else

#include <unistd.h>
#include <fcntl.h>
typedef int TcpSocket;

#endif


namespace MT
{
	class TaskScheduler;

	namespace profile
	{

		class MicroWebServer
		{

			struct SocketMode
			{
				enum Type
				{
					BLOCKING = 0,
					NONBLOCKING = 1,
				};
			};

			TcpSocket listenerSocket;

			void Cleanup();
			void CloseSocket(TcpSocket socket);
			void SetSocketMode(TcpSocket socket, SocketMode::Type mode);

			static bool IsValidSocket(TcpSocket socket);
			static bool StringEqualCaseInsensitive(const char * str1, const char * str2);
			static char* FindSubstring(char * str, char * substr);

			char * requestData;

			void Append(const char * txt);
			const char * StringFormat(const char * formatString, ...);

			char * answerData;
			uint32 answerSize;

			char* stringFormatBuffer;

		public:

			MicroWebServer();
			~MicroWebServer();

			int32 Serve(uint16 portRangeMin, uint16 portRangeMax);
			void Update(MT::TaskScheduler & scheduler);
		};
	}

}

#endif