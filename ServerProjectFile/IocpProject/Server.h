#pragma once
#pragma comment(lib, "ws2_32")

#include <iostream>
#include<WinSock2.h>
#include <tchar.h>

#define PORT 9000

class Server
{
	WSADATA wsa;
	SOCKET listen_sock;
	SOCKADDR_IN ServerAddress;

public:
	Server();

	VOID errQuit(const TCHAR* msg);
	VOID errDisplay(const TCHAR* msg);
	VOID setReady();
	VOID Connect();
	VOID ProcessClient();

	~Server();
};