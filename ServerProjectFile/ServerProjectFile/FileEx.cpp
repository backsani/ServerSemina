#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>

void _tmain(void) {
    DWORD value[50] = { 0 };
    srand((unsigned int)time(0)); // 시드 값을 명시적으로 unsigned int로 캐스팅
    DWORD dwWroteBytes;

    // 50개의 랜덤 값을 생성하여 value 배열에 저장
    for (int i = 0; i < 50; i++) {
        value[i] = rand() % 10;
    }
    value[0] = 100; // 이 값은 테스트용으로 변경하신 것 같습니다.

    // Data.txt 파일을 생성 또는 열기
    HANDLE hFile = CreateFile(_T("Data.txt"), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Create File failed, code: %d\n", GetLastError());
        return;
    }
    else {
        printf("Create File success!\n");
    }

    // TempData.txt 파일을 임시 파일로 생성
    HANDLE tempFile = CreateFile(_T("TempData.txt"), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (tempFile == INVALID_HANDLE_VALUE) {
        printf("Create tempFile failed, code: %d\n", GetLastError());
        return;
    }
    else {
        printf("Create tempFile success!\n");
    }

    // value 배열을 텍스트 형식으로 변환하여 buffer에 저장
    wchar_t buffer[256];
    int offset = 0;
    for (int i = 0; i < 50; i++) {
        offset += swprintf(buffer + offset, sizeof(buffer) - offset, L"%d ", value[i]);
    }

    printf("offset value: %d\n", offset);

    // buffer의 내용을 파일에 작성
    BOOL blsOK = WriteFile(hFile, buffer, offset * sizeof(wchar_t), &dwWroteBytes, NULL);
    if (!blsOK || dwWroteBytes != offset * sizeof(wchar_t)) {
        printf("데이터 작성 실패, code: %d\n", GetLastError());
        CloseHandle(hFile);
        CloseHandle(tempFile);
        return;
    }
    else {
        printf("데이터 작성 성공, 작성한 데이터 크기: %d bytes\n", dwWroteBytes);
    }

    // 파일 핸들 닫기
    CloseHandle(hFile);
    CloseHandle(tempFile);
}
