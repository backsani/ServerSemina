#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>

void _tmain(void) {
    DWORD value[50] = { 0 };
    srand((unsigned int)time(0)); // �õ� ���� ��������� unsigned int�� ĳ����
    DWORD dwWroteBytes;

    // 50���� ���� ���� �����Ͽ� value �迭�� ����
    for (int i = 0; i < 50; i++) {
        value[i] = rand() % 10;
    }
    value[0] = 100; // �� ���� �׽�Ʈ������ �����Ͻ� �� �����ϴ�.

    // Data.txt ������ ���� �Ǵ� ����
    HANDLE hFile = CreateFile(_T("Data.txt"), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Create File failed, code: %d\n", GetLastError());
        return;
    }
    else {
        printf("Create File success!\n");
    }

    // TempData.txt ������ �ӽ� ���Ϸ� ����
    HANDLE tempFile = CreateFile(_T("TempData.txt"), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (tempFile == INVALID_HANDLE_VALUE) {
        printf("Create tempFile failed, code: %d\n", GetLastError());
        return;
    }
    else {
        printf("Create tempFile success!\n");
    }

    // value �迭�� �ؽ�Ʈ �������� ��ȯ�Ͽ� buffer�� ����
    wchar_t buffer[256];
    int offset = 0;
    for (int i = 0; i < 50; i++) {
        offset += swprintf(buffer + offset, sizeof(buffer) - offset, L"%d ", value[i]);
    }

    printf("offset value: %d\n", offset);

    // buffer�� ������ ���Ͽ� �ۼ�
    BOOL blsOK = WriteFile(hFile, buffer, offset * sizeof(wchar_t), &dwWroteBytes, NULL);
    if (!blsOK || dwWroteBytes != offset * sizeof(wchar_t)) {
        printf("������ �ۼ� ����, code: %d\n", GetLastError());
        CloseHandle(hFile);
        CloseHandle(tempFile);
        return;
    }
    else {
        printf("������ �ۼ� ����, �ۼ��� ������ ũ��: %d bytes\n", dwWroteBytes);
    }

    // ���� �ڵ� �ݱ�
    CloseHandle(hFile);
    CloseHandle(tempFile);
}
