#include "Server.h"
#include <thread>


Server::Server() {
	// 윈속 초기화
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return;
}

Server::~Server()
{
	// closesocket()
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
}

VOID Server::errQuit(const TCHAR* msg) {
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, (LPCWSTR)msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

VOID Server::errDisplay(const TCHAR* msg) {
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", (LPCWSTR)msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

VOID Server::setReady() {
	INT retval;

	// socket()
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
		errDisplay(_T("socket()"));

	//bind()
	ZeroMemory(&ServerAddress, sizeof(SOCKADDR_IN));
	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	ServerAddress.sin_port = htons(PORT);
	retval = bind(listen_sock, (SOCKADDR*)&ServerAddress, sizeof(ServerAddress));
	if (retval == SOCKET_ERROR)
		//error_Quit(_T("Bind()"));
		return;

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) errQuit(_T("listen()"));
}

VOID Server::Connect() {
	SOCKET clientSocket;
	SOCKADDR_IN clientAddress;
	INT AddressLen;
	AddressLen = sizeof(clientAddress);


	while (1)
	{
		//Accept()
		clientSocket = accept(listen_sock, (SOCKADDR*)&clientAddress, &AddressLen);
		if (clientSocket == INVALID_SOCKET)
		{
			//errDisplay(_T("Accept"));
			continue;
		}

		std::thread t(&Server::ProcessClient, this, clientSocket);
		t.detach();

	}
}

VOID Server::ProcessClient() {

}
