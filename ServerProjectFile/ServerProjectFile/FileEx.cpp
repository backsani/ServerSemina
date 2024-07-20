#include <Windows.h>
#include <tchar.h>
#include <time.h>
#include <wchar.h>
#include <iostream>
#include <thread>
#include <vector>
#include <future>

ULONGLONG myGetFileSize(HANDLE hFile);
VOID ReadThread(HANDLE hFile, HANDLE tempFile, ULONGLONG fPointer, DWORD ReadSize, std::promise<bool>&& promise);

void _tmain(void) {
    DWORD value[30] = { 0 };
    srand((unsigned int)time(0)); // �õ� ���� ��������� unsigned int�� ĳ����
    DWORD dwWroteBytes;

    // 50���� ���� ���� �����Ͽ� value �迭�� ����
    for (int i = 0; i < 30; i++) {
        value[i] = rand() % 10;
    }

    // Data.txt ������ ���� �Ǵ� ����
    HANDLE hFile = CreateFile(_T("Data.txt"), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cout << "Create File failed, code : " << GetLastError() << std::endl;
        return;
    }
    else {
        std::cout << "Create File success!\n" << std::endl;
    }

    // TempData.txt ������ �ӽ� ���Ϸ� ����
    HANDLE tempFile = CreateFile(_T("TempData.txt"), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (tempFile == INVALID_HANDLE_VALUE) {
        std::cout << "Create tempFile failed, code : " << GetLastError() << std::endl;
        return;
    }
    else {
        std::cout << "Create tempFile success!\n" << std::endl;
    }

    // value �迭�� �ؽ�Ʈ �������� ��ȯ�Ͽ� buffer�� ����
    wchar_t buffer[256];
    int offset = 0;
    for (int i = 0; i < 30; i++) {
        offset += swprintf(buffer + offset, sizeof(buffer) / sizeof(wchar_t) - offset, L"%d ", value[i]);
    }

    std::cout << "offset value : " << offset << std::endl;

    // buffer�� ������ ���Ͽ� �ۼ�
    BOOL blsOK = WriteFile(hFile, buffer, offset * sizeof(wchar_t), &dwWroteBytes, NULL);
    if (!blsOK || dwWroteBytes != offset * sizeof(wchar_t)) {
        std::cout << "������ �ۼ� ����, code: \n" << GetLastError() << std::endl;
        CloseHandle(hFile);
        CloseHandle(tempFile);
        return;
    }
    else {
        std::cout << "������ �ۼ� ����, �ۼ��� ������ ũ��: " << dwWroteBytes << " bytes\n" << std::endl;
    }

    ULONGLONG fSize = myGetFileSize(hFile);
    DWORD readSize = (fSize / 3);

    std::promise<bool> promise[3];
    std::future<bool> future[3];
    for (int i = 0; i < 3; i++) {
        future[i] = promise[i].get_future();
    }
    //std::vector<std::thread> thread;

    std::thread t1(ReadThread, hFile, tempFile, 0, readSize, std::move(promise[0]));
    std::thread t2(ReadThread, hFile, tempFile, fSize / 3, readSize, std::move(promise[1]));
    std::thread t3(ReadThread, hFile, tempFile, (fSize / 3) * 2, readSize, std::move(promise[2]));
    t1.detach();
    t2.detach();
    t3.detach();

    /*t1.join();
    t2.join();
    t3.join();*/

    for (int i = 0; i < 3; i++) {
        if (!future[i].get()) {
            //ReadThread ���� �߻�
            CloseHandle(hFile);
            CloseHandle(tempFile);
            return;
        }
    }  

    Sleep(5000);

    DWORD bytesRead;
    wchar_t tempBuffer[256];

    if (!ReadFile(tempFile, tempBuffer, (fSize * sizeof(wchar_t)), &bytesRead, NULL)) {
        std::cout << "temp ���� �б� ����" << std::endl;
        CloseHandle(hFile);
        CloseHandle(tempFile);
        return;
    }

    std::wcout << "���� ���� ������ : " << tempBuffer << " ���� ũ�� : " << bytesRead << '\n' << std::endl;

    // ���� �ڵ� �ݱ�
    CloseHandle(hFile);
    CloseHandle(tempFile);
}

VOID ReadThread(HANDLE hFile, HANDLE tempFile, ULONGLONG fPointer, DWORD ReadSize, std::promise<bool> && promise) {
    //���� �����͸� �̵��� ������
    LARGE_INTEGER liDistanceToMove;
    liDistanceToMove.QuadPart = fPointer;

    wchar_t buffer[256];

    LARGE_INTEGER liNewFilePointer;
    if (!SetFilePointerEx(hFile, liDistanceToMove, &liNewFilePointer, FILE_BEGIN)) {
        std::cout << "���� ������ �̵� ����" << std::endl;
        // ���� �ڵ� �ݱ�
        promise.set_value(false);
        return;
    }
    //(sizeof(buffer) - sizeof(wchar_t)) / 3

    DWORD bytesRead;
    if (!ReadFile(hFile, buffer, ReadSize, &bytesRead, NULL)) {
        std::cout << "���� �б� ����" << std::endl;
        promise.set_value(false);
        return;
    }

    if (!FlushFileBuffers(hFile)) {
        std::cout << "FlushFileBuffers failed , code : " << GetLastError() << std::endl;
    }
    
    buffer[bytesRead / sizeof(wchar_t)] = L'\0';

    std::wcout << "���� ������ : " << buffer << std::endl;

    if (!SetFilePointerEx(tempFile, liDistanceToMove, &liNewFilePointer, FILE_BEGIN)) {
        std::cout << "���� ������ �̵� ����" << std::endl;
        // ���� �ڵ� �ݱ�
        promise.set_value(false);
        return;
    }

    DWORD dwWroteBytes;

    // ����� ���� OVERLAPPED ����ü �ʱ�ȭ
    OVERLAPPED ol = { 0 };
    ol.Offset = 0;
    ol.OffsetHigh = 0;

    if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, ReadSize * 3, 0, &ol)) {
        std::cerr << "LockFileEx failed, code: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return ;
    }

    // buffer�� ������ ���Ͽ� �ۼ�
    BOOL blsOK = WriteFile(hFile, buffer, bytesRead, &dwWroteBytes, NULL);
    if (!blsOK) {
        std::cout << "������ �ۼ� ����, code: \n" << GetLastError() << std::endl;
        CloseHandle(hFile);
        CloseHandle(tempFile);
        return;
    }
    else {
        std::cout << "������ ���� ����, �ۼ��� ������ ũ��: " << dwWroteBytes << " bytes\n" << std::endl;
    }

    // ��� ����
    if (!UnlockFileEx(hFile, 0, ReadSize * 3, 0, &ol)) {
        std::cerr << "UnlockFileEx failed, code: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return ;
    }

    promise.set_value(true);
    return;
}


ULONGLONG myGetFileSize(HANDLE hFile) {
    // ���� ũ�⸦ �������� ���� ����
    DWORD fileSizeLow;
    DWORD fileSizeHigh;

    fileSizeLow = GetFileSize(hFile, &fileSizeHigh);

    if (fileSizeLow == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
        printf("���� ũ�⸦ �������µ� �����߽��ϴ�.");
        CloseHandle(hFile);
        return 0;
    }

    // ���� ũ�� ���
    ULONGLONG fileSize = ((ULONGLONG)fileSizeHigh << 32) | fileSizeLow;
    printf("���� ũ�� : %llu bytes\n", fileSize);

    return fileSize;
}