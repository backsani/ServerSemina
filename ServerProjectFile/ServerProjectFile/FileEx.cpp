#include <Windows.h>
#include <tchar.h>
#include <time.h>
#include <wchar.h>
#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx;

ULONGLONG myGetFileSize(HANDLE hFile);
VOID ReadThread(HANDLE hFile, HANDLE tempFile, ULONGLONG fPointer, DWORD ReadSize);

void _tmain(void) {
    DWORD value[30] = { 0 };
    srand((unsigned int)time(0)); // 시드 값을 명시적으로 unsigned int로 캐스팅
    DWORD dwWroteBytes;

    // 50개의 랜덤 값을 생성하여 value 배열에 저장
    for (int i = 0; i < 30; i++) {
        value[i] = rand() % 10;
    }

    // Data.txt 파일을 생성 또는 열기
    HANDLE hFile = CreateFile(_T("Data.txt"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cout << "Create File failed, code : " << GetLastError() << std::endl;
        return;
    }
    else {
        std::cout << "Create File success!\n" << std::endl;
    }

    // TempData.txt 파일을 임시 파일로 생성
    HANDLE tempFile = CreateFile(_T("TempData.txt"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (tempFile == INVALID_HANDLE_VALUE) {
        std::cout << "Create tempFile failed, code : " << GetLastError() << std::endl;
        return;
    }
    else {
        std::cout << "Create tempFile success!\n" << std::endl;
    }

    // value 배열을 텍스트 형식으로 변환하여 buffer에 저장
    wchar_t buffer[256] = { 0 };
    int offset = 0;
    for (int i = 0; i < 30; i++) {
        offset += swprintf(buffer + offset, sizeof(buffer) / sizeof(wchar_t) - offset, L"%d ", value[i]);
    }

    std::cout << "offset value : " << offset << std::endl;

    // buffer의 내용을 파일에 작성
    BOOL blsOK = WriteFile(hFile, buffer, offset * sizeof(wchar_t), &dwWroteBytes, NULL);
    if (!blsOK || dwWroteBytes != offset * sizeof(wchar_t)) {
        std::cout << "데이터 작성 실패, code: \n" << GetLastError() << std::endl;
        CloseHandle(hFile);
        CloseHandle(tempFile);
        return;
    }
    else {
        std::cout << "데이터 작성 성공, 작성한 데이터 크기: " << dwWroteBytes << " bytes\n" << std::endl;
    }

    ULONGLONG fSize = myGetFileSize(hFile);
    DWORD readSize = (fSize / 3);

    std::thread t1(ReadThread, hFile, tempFile, 0, readSize);
    std::thread t2(ReadThread, hFile, tempFile, fSize / 3, readSize);
    std::thread t3(ReadThread, hFile, tempFile, (fSize / 3) * 2, readSize);

    t1.join();
    t2.join();
    t3.join();


    // 임시 파일에서 데이터 읽기 전에 FlushFileBuffers 호출
    if (!FlushFileBuffers(tempFile)) {
        std::cout << "FlushFileBuffers failed, code: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        CloseHandle(tempFile);
        return;
    }
 
    LARGE_INTEGER zero;
    zero.QuadPart = 0;
    if (!SetFilePointerEx(tempFile, zero, NULL, FILE_BEGIN)) {
        std::cout << "SetFilePointerEx failed, code: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        CloseHandle(tempFile);
        return;
    }

    DWORD bytesRead;
    wchar_t tempBuffer[256] = { 0 };

    if (!ReadFile(tempFile, tempBuffer, (fSize), &bytesRead, NULL)) {
        std::cout << "temp 파일 읽기 실패" << std::endl;
        CloseHandle(hFile);
        CloseHandle(tempFile);
        return;
    }

    std::wcout << "읽은 파일 데이터 : " << tempBuffer << " 읽은 크기 : " << bytesRead << std::endl;

    // 파일 핸들 닫기
    CloseHandle(hFile);
    CloseHandle(tempFile);
}

VOID ReadThread(HANDLE hFile, HANDLE tempFile, ULONGLONG fPointer, DWORD ReadSize) {
    //파일 포인터를 이동할 오프셋
    LARGE_INTEGER liDistanceToMove;
    liDistanceToMove.QuadPart = fPointer;

    wchar_t buffer[256];

    LARGE_INTEGER liNewFilePointer;
    if (!SetFilePointerEx(hFile, liDistanceToMove, &liNewFilePointer, FILE_BEGIN)) {
        std::cout << "파일 포인터 이동 실패" << std::endl;
        return;
    }
    //(sizeof(buffer) - sizeof(wchar_t)) / 3

    DWORD bytesRead;
    if (!ReadFile(hFile, buffer, ReadSize, &bytesRead, NULL)) {
        std::cout << "파일 읽기 실패" << std::endl;
        return;
    }

    if (!FlushFileBuffers(hFile)) {
        std::cout << "FlushFileBuffers failed , code : " << GetLastError() << std::endl;
        
    }
    
    buffer[bytesRead / sizeof(wchar_t)] = L'\0';

    std::unique_lock<std::mutex> lock(mtx);

    std::wcout << "읽은 데이터 : " << buffer << std::endl;

    if (!SetFilePointerEx(tempFile, liDistanceToMove, &liNewFilePointer, FILE_BEGIN)) {
        std::cout << "파일 포인터 이동 실패" << std::endl;
        return;
    }

    DWORD dwWroteBytes;

    // buffer의 내용을 파일에 작성
    BOOL blsOK = WriteFile(tempFile, buffer, bytesRead, &dwWroteBytes, NULL);
    if (!blsOK) {
        std::cout << "데이터 작성 실패, code: \n" << GetLastError() << std::endl;
        CloseHandle(hFile);
        CloseHandle(tempFile);
        return;
    }
    else {
        std::cout << "데이터 복사 성공, 작성한 데이터 크기: " << dwWroteBytes << " bytes\n" << std::endl;
    }

    lock.unlock();


    return;
}


ULONGLONG myGetFileSize(HANDLE hFile) {
    // 파일 크기를 가져오기 위한 변수
    DWORD fileSizeLow;
    DWORD fileSizeHigh;

    fileSizeLow = GetFileSize(hFile, &fileSizeHigh);

    if (fileSizeLow == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
        printf("파일 크기를 가져오는데 실패했습니다.");
        CloseHandle(hFile);
        return 0;
    }

    // 파일 크기 출력
    ULONGLONG fileSize = ((ULONGLONG)fileSizeHigh << 32) | fileSizeLow;
    printf("파일 크기 : %llu bytes\n", fileSize);

    return fileSize;
}