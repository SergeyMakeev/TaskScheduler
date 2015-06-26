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

#ifdef MT_INSTRUMENTED_BUILD

#define _CRT_SECURE_NO_WARNINGS

#include <MTMicroWebSrv.h>
#include <MTThreadContext.h>
#include <MTScheduler.h>

#include "malloc.h"

#define MAX_REQUEST_SIZE (8192)
#define MAX_ANSWER_SIZE (4*128*1024)
#define MAX_STRINGFORMAT_BUFFER_SIZE (65536)

#ifdef _WIN32

#include <winsock2.h>
#define BAD_SOCKET (INVALID_SOCKET)

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <string.h>
#include <strings.h>
#include <alloca.h>
#include <stdarg.h>
#define BAD_SOCKET (-1)

#endif


#define WEB_HTTP "HTTP/"
#define WEB_GET "GET /"


namespace MT
{

namespace profile
{


MicroWebServer::MicroWebServer()
	: listenerSocket(BAD_SOCKET)
{
#ifdef _WIN32
	WSADATA wsa;
	int res = WSAStartup(MAKEWORD(2, 2), &wsa);
	MT_ASSERT(res == 0, "Can't WSAStartup");
#endif

	requestData = (char*)malloc(MAX_REQUEST_SIZE);
	answerData = (char*)malloc(MAX_ANSWER_SIZE);
	stringFormatBuffer = (char*)malloc(MAX_STRINGFORMAT_BUFFER_SIZE);
}

MicroWebServer::~MicroWebServer()
{
	Cleanup();

#ifdef _WIN32
	WSACleanup();
#endif

	free(requestData);
	free(answerData);
	free(stringFormatBuffer);

	requestData = nullptr;
	answerData = nullptr;
	stringFormatBuffer = nullptr;
}

bool MicroWebServer::IsValidSocket(TcpSocket socket)
{
#ifdef _WIN32
	if (socket == BAD_SOCKET)
	{
		return false;
	}
#else
	if (socket < 0)
	{
		return false;
	}
#endif
	return true;
}

char* MicroWebServer::FindSubstring(char * str, char * substr)
{
	char* res = strstr(str, substr);
	return res;
}

bool MicroWebServer::StringEqualCaseInsensitive(const char * str1, const char * str2)
{
#ifdef _WIN32
	return (_stricmp(str1, str2) == 0);
#else
	return (strcasecmp(str1, str2) == 0);
#endif
}

void MicroWebServer::SetSocketMode(TcpSocket socket, SocketMode::Type mode)
{
#ifdef _WIN32
	u_long nonBlocking = (mode == SocketMode::NONBLOCKING) ? 1 : 0;
	ioctlsocket(socket, FIONBIO, &nonBlocking);
#else
	int socketOptions = fcntl(socket, F_GETFL);
	if(mode == SocketMode::NONBLOCKING)
	{
		fcntl(socket, F_SETFL, socketOptions | O_NONBLOCK);
	} else
	{
		fcntl(socket, F_SETFL, socketOptions & (~O_NONBLOCK));
	}
#endif
}

void MicroWebServer::CloseSocket(TcpSocket socket)
{
	if (!IsValidSocket(socket))
	{
		return;
	}

#ifdef _WIN32
		closesocket(socket);
#else
		close(socket);
#endif
}

void MicroWebServer::Cleanup()
{
	CloseSocket(listenerSocket);
	listenerSocket = BAD_SOCKET;
}

int32 MicroWebServer::Serve(uint16 portRangeMin, uint16 portRangeMax)
{
	MT_ASSERT(portRangeMin <= portRangeMax, "Invalid port range");

	listenerSocket = socket(PF_INET, SOCK_STREAM, 6);
	SetSocketMode(listenerSocket, SocketMode::NONBLOCKING);

	int32 webServerPort = -1;

	struct sockaddr_in Addr;
	Addr.sin_family = AF_INET;
	Addr.sin_addr.s_addr = INADDR_ANY;
	for(uint16 port = portRangeMin; port <= portRangeMax; port++)
	{
		Addr.sin_port = htons((u_short)port);
		if(bind(listenerSocket, (sockaddr*)&Addr, sizeof(Addr)) == 0)
		{
			webServerPort = port;
			break;
		}
	}

	if (webServerPort < 0)
	{
		Cleanup();
		return -1;
	}

	listen(listenerSocket, 8);
	return webServerPort;
}

void MicroWebServer::AppendFromFile(FILE* file)
{
	fseek(file, 0, SEEK_END);
	int fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (answerSize + fileSize + 1 >= MAX_ANSWER_SIZE)
	{
		return;
	}

	char* const pBuffer = answerData + answerSize;

	int nRead = (int)fread(pBuffer, 1, fileSize, file);
	MT_ASSERT(nRead == fileSize, "File read error");
	fclose(file);
	answerSize += fileSize;
	answerData[answerSize] = '\0';
}

void MicroWebServer::Append(const char * txt)
{
	size_t stringLen = strlen(txt);

	if (answerSize + stringLen + 1 >= MAX_ANSWER_SIZE)
	{
		return;
	}

	memcpy(answerData + answerSize, txt, stringLen);
	answerSize += stringLen;
	answerData[answerSize] = '\0';
}

const char * MicroWebServer::StringFormat(const char * formatString, ...)
{
	va_list va;
	va_start( va, formatString );
#ifdef _WIN32
	_vsnprintf_s( stringFormatBuffer, MAX_STRINGFORMAT_BUFFER_SIZE, MAX_STRINGFORMAT_BUFFER_SIZE - 1, formatString, va );
#else
	vsprintf( stringFormatBuffer, formatString, va );
#endif // _WIN32
	va_end( va );
	return stringFormatBuffer;
}

void MicroWebServer::Update(MT::TaskScheduler & scheduler)
{
	MT::ProfileEventDesc eventsBuffer[4096];

	TcpSocket clientSocket = accept(listenerSocket, 0, 0);
	SetSocketMode(clientSocket, SocketMode::BLOCKING);

	int numBytesReceived = recv(clientSocket, requestData, (MAX_REQUEST_SIZE-1), 0);
	if (numBytesReceived > 0)
	{
		answerSize = 0;
		answerData[0] = '\0';
		requestData[numBytesReceived] = '\0';

		char* pHttp = FindSubstring(requestData, WEB_HTTP);
		char* pGet = FindSubstring(requestData, WEB_GET);

		if (pHttp && pGet)
		{
			//document uri
			char* pDocument = pGet + sizeof(WEB_GET) - 1;
			char* pDocumentExt = pDocument;
			for(char* ch = pDocument;; ch++)
			{
				if (*ch == ' ' || *ch == '\0')
				{
					*ch = '\0';
					break;
				} else
				{
					if (*ch == '.')
					{
						pDocumentExt = ch;
					}

				}
			}

			if (StringEqualCaseInsensitive(pDocument, "data.json"))
			{
				Append("HTTP/1.0 200 OK\r\nContent-Type: application/json \r\n\r\n");

				Append("{");

				uint32 threadCount = scheduler.GetWorkerCount();

				Append("\"threads\": [");

				for(uint32 workerId = 0; workerId < threadCount; workerId++)
				{
					Append("{");

					size_t eventsCount = scheduler.GetProfilerEvents(workerId, &eventsBuffer[0], MT_ARRAY_SIZE(eventsBuffer));

					Append("\"events\": [");

					for(size_t eventId = 0; eventId < eventsCount; eventId++)
					{
						Append("{");

						MT::ProfileEventDesc evt = eventsBuffer[eventId];

						Append(StringFormat("\"time\" : %llu,", evt.timeStampMicroSeconds));
						Append(StringFormat("\"type\" : %d,", evt.type));
						Append(StringFormat("\"id\" : \"%s\",", evt.id));
						Append(StringFormat("\"color\" : %d", evt.colorIndex));

						if ((eventId + 1) < eventsCount)
						{
							Append("},");
						} else
						{
							Append("}");
						}
					}

					Append("]");

					if ((workerId + 1) < threadCount)
					{
						Append("},");
					} else
					{
						Append("}");
					}
				}

				Append("]");

				Append("}");
			} else
			{
				if (pDocument[0] == '\0')
				{
					pDocument = "Profiler.html";
				}

				// profiler web page
				FILE* file = fopen(pDocument, "rb");
				if (file != nullptr)
				{

					if (pDocumentExt[0] == '.' && pDocumentExt[1] == 'c' && pDocumentExt[2] == 's' && pDocumentExt[3] == 's')
					{
						Append("HTTP/1.0 200 OK\r\nContent-Type: text/css \r\n\r\n");
					} else
					{
						if (pDocumentExt[0] == '.' && pDocumentExt[1] == 'j' && pDocumentExt[2] == 's')
						{
							Append("HTTP/1.0 200 OK\r\nContent-Type: text/javascript \r\n\r\n");
						} else
						{
							Append("HTTP/1.0 200 OK\r\nContent-Type: text/html \r\n\r\n");
						}
					}

					AppendFromFile(file);

				} else
				{
					Append("HTTP/1.1 404 Not Found\r\n\r\nError 404. File '");
					Append(pDocument);
					Append("' Not Found.");
				}
			}

			send(clientSocket, answerData, answerSize, 0);
		}
	}

	CloseSocket(clientSocket);
	clientSocket = BAD_SOCKET;
}

}

}


#endif
