#include <iostream>
#include "Server.h"

INT _tmain(INT argc, TCHAR* argv[]) {
	Server server;
	server.setReady();
	server.Connect();

	return 0;
}