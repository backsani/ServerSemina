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
    srand((unsigned int)time(0)); // 시드 값을 명시적으로 unsigned int로 캐스팅
    DWORD dwWroteBytes;

    // 50개의 랜덤 값을 생성하여 value 배열에 저장
    for (int i = 0; i < 30; i++) {
        value[i] = rand() % 10;
    }

    // Data.txt 파일을 생성 또는 열기
    HANDLE hFile = CreateFile(_T("Data.txt"), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cout << "Create File failed, code : " << GetLastError() << std::endl;
        return;
    }
    else {
        std::cout << "Create File success!\n" << std::endl;
    }

    // TempData.txt 파일을 임시 파일로 생성
    HANDLE tempFile = CreateFile(_T("TempData.txt"), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (tempFile == INVALID_HANDLE_VALUE) {
        std::cout << "Create tempFile failed, code : " << GetLastError() << std::endl;
        return;
    }
    else {
        std::cout << "Create tempFile success!\n" << std::endl;
    }

    // value 배열을 텍스트 형식으로 변환하여 buffer에 저장
    wchar_t buffer[256];
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
            //ReadThread 오류 발생
            CloseHandle(hFile);
            CloseHandle(tempFile);
            return;
        }
    }  

    Sleep(5000);

    DWORD bytesRead;
    wchar_t tempBuffer[256];

    if (!ReadFile(tempFile, tempBuffer, (fSize * sizeof(wchar_t)), &bytesRead, NULL)) {
        std::cout << "temp 파일 읽기 실패" << std::endl;
        CloseHandle(hFile);
        CloseHandle(tempFile);
        return;
    }

    std::wcout << "읽은 파일 데이터 : " << tempBuffer << " 읽은 크기 : " << bytesRead << '\n' << std::endl;

    // 파일 핸들 닫기
    CloseHandle(hFile);
    CloseHandle(tempFile);
}

VOID ReadThread(HANDLE hFile, HANDLE tempFile, ULONGLONG fPointer, DWORD ReadSize, std::promise<bool> && promise) {
    //파일 포인터를 이동할 오프셋
    LARGE_INTEGER liDistanceToMove;
    liDistanceToMove.QuadPart = fPointer;

    wchar_t buffer[256];

    LARGE_INTEGER liNewFilePointer;
    if (!SetFilePointerEx(hFile, liDistanceToMove, &liNewFilePointer, FILE_BEGIN)) {
        std::cout << "파일 포인터 이동 실패" << std::endl;
        // 파일 핸들 닫기
        promise.set_value(false);
        return;
    }
    //(sizeof(buffer) - sizeof(wchar_t)) / 3

    DWORD bytesRead;
    if (!ReadFile(hFile, buffer, ReadSize, &bytesRead, NULL)) {
        std::cout << "파일 읽기 실패" << std::endl;
        promise.set_value(false);
        return;
    }

    if (!FlushFileBuffers(hFile)) {
        std::cout << "FlushFileBuffers failed , code : " << GetLastError() << std::endl;
    }
    
    buffer[bytesRead / sizeof(wchar_t)] = L'\0';

    std::wcout << "읽은 데이터 : " << buffer << std::endl;

    if (!SetFilePointerEx(tempFile, liDistanceToMove, &liNewFilePointer, FILE_BEGIN)) {
        std::cout << "파일 포인터 이동 실패" << std::endl;
        // 파일 핸들 닫기
        promise.set_value(false);
        return;
    }

    DWORD dwWroteBytes;

    // 잠금을 위한 OVERLAPPED 구조체 초기화
    OVERLAPPED ol = { 0 };
    ol.Offset = 0;
    ol.OffsetHigh = 0;

    if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, ReadSize * 3, 0, &ol)) {
        std::cerr << "LockFileEx failed, code: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return ;
    }

    // buffer의 내용을 파일에 작성
    BOOL blsOK = WriteFile(hFile, buffer, bytesRead, &dwWroteBytes, NULL);
    if (!blsOK) {
        std::cout << "데이터 작성 실패, code: \n" << GetLastError() << std::endl;
        CloseHandle(hFile);
        CloseHandle(tempFile);
        return;
    }
    else {
        std::cout << "데이터 복사 성공, 작성한 데이터 크기: " << dwWroteBytes << " bytes\n" << std::endl;
    }

    // 잠금 해제
    if (!UnlockFileEx(hFile, 0, ReadSize * 3, 0, &ol)) {
        std::cerr << "UnlockFileEx failed, code: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return ;
    }

    promise.set_value(true);
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