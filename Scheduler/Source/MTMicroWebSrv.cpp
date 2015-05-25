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

#include "malloc.h"

#define MAX_REQUEST_SIZE (8192)
#define MAX_ANSWER_SIZE (32768)

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
#define BAD_SOCKET (-1)

#endif


namespace MT
{

MicroWebServer::MicroWebServer()
	: listenerSocket(BAD_SOCKET)
{
#ifdef _WIN32
	WSADATA wsa;
	int res = WSAStartup(MAKEWORD(2, 2), &wsa);
	ASSERT(res == 0, "Can't WSAStartup");
#endif


	requestData = (char*)malloc(MAX_REQUEST_SIZE);
	answerData = (char*)malloc(MAX_ANSWER_SIZE);
}



MicroWebServer::~MicroWebServer()
{
	Cleanup();

#ifdef _WIN32
	WSACleanup();
#endif

	free(requestData);
	free(answerData);

	requestData = nullptr;
	answerData = nullptr;
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
	return (strcasecmp(str1, str2) == 0)
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
	ASSERT(portRangeMin <= portRangeMax, "Invalid port range");

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


#define WEB_HTTP "HTTP/"
#define WEB_GET "GET /"

void MicroWebServer::Update()
{
	TcpSocket clientSocket = accept(listenerSocket, 0, 0);
	if (!IsValidSocket(clientSocket))
	{
		return;
	}

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
			for(char* ch = pDocument;; ch++)
			{
				if (*ch == ' ' || *ch == '\0')
				{
					*ch = '\0';
					break;
				}
			}

			if (StringEqualCaseInsensitive(pDocument, "data.json"))
			{
				Append("HTTP/1.0 200 OK\r\nContent-Type: application/json \r\n\r\n");
				Append("This is data json");
			} else
			{
				// profiler web page
				FILE* file = fopen("profiler.html", "rb");
				if (file != nullptr)
				{
					fseek(file, 0, SEEK_END);
					int fileSize = ftell(file);
					fseek(file, 0, SEEK_SET);

					Append("HTTP/1.0 200 OK\r\nContent-Type: text/html \r\n\r\n");

					char* const pBuffer = (char*)alloca(fileSize+1);
					int nRead = (int)fread(pBuffer, 1, fileSize, file);
					ASSERT(nRead == fileSize, "File read error");
					fclose(file);
					pBuffer[fileSize] = '\0';

					Append(pBuffer);
				} else
				{
					Append("HTTP/1.1 404 Not Found\r\n\r\nError 404. Page Not Found.");
				}
			}

			send(clientSocket, answerData, answerSize, 0);
		}
	}

	CloseSocket(clientSocket);
	clientSocket = BAD_SOCKET;
}


}


#endif