#include <iostream>
#include <tchar.h>
#include <Windows.h>
#include <thread>

#define BUFF_SIZE 65536
#define READ_KEY 1

struct CHUNCK : OVERLAPPED
{
	HANDLE _hFile;
	wchar_t buffer[BUFF_SIZE];

	CHUNCK(HANDLE hFile) {
		memset(this, 0, sizeof(*this));
		_hFile = hFile;
	}
}; typedef CHUNCK* PCHUNCK;

struct READ_ENV
{
	HANDLE _hIocp;
	LONG _nCpCnt;
	HANDLE _hevEnd;
}; typedef READ_ENV* PENV;

DWORD WINAPI IOCPReadProc(PVOID pParam);

INT _tmain(void) {
	PCHUNCK Chunck[4];
	memset(Chunck, 0, sizeof(PCHUNCK) * 4);

	READ_ENV env;
	env._nCpCnt = 0;
	env._hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);
	env._hevEnd = CreateEvent(NULL, TRUE, FALSE, NULL);

	HANDLE hFile[4];
	hFile[0] = CreateFile(_T("file1.txt"), GENERIC_READ , FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
	hFile[1] = CreateFile(_T("file2.txt"), GENERIC_READ , FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
	hFile[2] = CreateFile(_T("file3.txt"), GENERIC_READ , FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
	hFile[3] = CreateFile(_T("file4.txt"), GENERIC_READ , FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);

	for (int i = 0; i < 4; i++) {
		if (hFile[i] == INVALID_HANDLE_VALUE) {
			std::cout << "Open i File failed, code : " << GetLastError() << std::endl;
			return 0;
		}
		else {
			std::cout << "Open i File success!\n" << std::endl;
		}

		CreateIoCompletionPort(hFile[i], env._hIocp, READ_KEY, 0);

		PCHUNCK pcc = new CHUNCK(hFile[i]);
		Chunck[i] = pcc;
		env._nCpCnt++;
	}
	
	LONG lChnCnt = env._nCpCnt;
	DWORD dwThrID = 0;
	HANDLE workThread[2];
	for (int i = 0; i < 2; i++) {
		workThread[i] = CreateThread(NULL, 0, IOCPReadProc, &env, 0, &dwThrID);
	}

	for (int i = 0; i < lChnCnt; i++) {
		PCHUNCK pcc = Chunck[i];
		if (!ReadFile(pcc->_hFile, pcc->buffer, BUFF_SIZE, NULL, pcc)) {
			DWORD dwErrCode = GetLastError();
			if (dwErrCode != ERROR_IO_PENDING)
				break;
		}
	}

	WaitForSingleObject(env._hevEnd, INFINITE);

	CloseHandle(env._hIocp);

	WaitForMultipleObjects(2, workThread, TRUE, INFINITE);

	for (int i = 0; i < lChnCnt; i++) {
		PCHUNCK pcc = Chunck[i];
		delete pcc;
	}
	for (int i = 0; i < 2; i++)
		CloseHandle(workThread[i]);
	CloseHandle(env._hevEnd);

	return 0;
}

DWORD WINAPI IOCPReadProc(PVOID pParam) {
	PENV pEnv = (PENV)pParam;
	DWORD dwThrId = GetCurrentThreadId();

	while (true) {
		DWORD dwErrCode = 0;
		PCHUNCK pcc = NULL;
		DWORD dwTrBytes = 0;
		ULONG_PTR ulkey;

		BOOL bIsOK = GetQueuedCompletionStatus(pEnv->_hIocp, &dwTrBytes, &ulkey, (LPOVERLAPPED*)&pcc, INFINITE);

		if (!bIsOK) {
			if (pcc == NULL)
				break;
			dwErrCode = GetLastError();
			goto $LABEL_CLOSE;
		}

		if (ulkey == READ_KEY)
		{
			//pcc->buffer[dwTrBytes / sizeof(wchar_t)] = L'\0';

			//std::cout << "Read File Success : " << pcc->buffer << std::endl;
			printf("Read File Success : %s\n", pcc->buffer);
		}

		continue;

	$LABEL_CLOSE:
		if (dwErrCode == ERROR_HANDLE_EOF)
			std::cout << dwThrId << "Thread Read success" << std::endl;
		else
			std::cout << dwThrId << "Thread Read failed, code : " << dwErrCode << std::endl;

		CloseHandle(pcc->_hFile);

		if (InterlockedDecrement(&pEnv->_nCpCnt) == 0)
			SetEvent(pEnv->_hevEnd);
	}

	return 0;
}