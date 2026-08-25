#define UNICODE

#include <windows.h>

void ShowErrorBox(LSTATUS statusCode) {
    LPTSTR errorText = NULL;

    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        statusCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&errorText,
        0,
        NULL
    );

    if (errorText != NULL) {
        MessageBoxW(NULL, errorText, L"Error", MB_OK | MB_ICONERROR);

        LocalFree(errorText);
        errorText = NULL;
    }
}

void CheckRegistry(HKEY hKey) {
    DWORD dwBufferSize(sizeof(DWORD));
    DWORD result(0);
    LONG status = RegQueryValueExW(
        hKey,
        L"ShutdownFlyoutOptions",
        NULL,
        NULL,
        reinterpret_cast<LPBYTE>(&result),
        &dwBufferSize
    );

    if (status != ERROR_SUCCESS) {
        ShowErrorBox(status);
        return;
    }

    if (result != 0 && (result & 0b0101) != 0b0101) {
        DWORD valueToSet = result | 0b0101;
        LSTATUS setError = RegSetValueExW(
            hKey,
            L"ShutdownFlyoutOptions",
            0,
            REG_DWORD,
            reinterpret_cast<const BYTE*>(&valueToSet),
            sizeof(valueToSet)
        );
        if (setError != ERROR_SUCCESS) {
            ShowErrorBox(setError);
            return;
        }
    }
}

int main() {
    HKEY hKey = nullptr;
    LSTATUS openStatus = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\WindowsUpdate\\Orchestrator",
        0,
        KEY_READ | KEY_NOTIFY | KEY_SET_VALUE,
        &hKey
    );

    if (openStatus != ERROR_SUCCESS) {
        ShowErrorBox(openStatus);
        return 1;
    }

    CheckRegistry(hKey);

    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hEvent == NULL) {
        ShowErrorBox(GetLastError());
        return 1;
    }

    while (true) {
        LONG lErrorCode = RegNotifyChangeKeyValue(hKey, FALSE, REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE);
        if (lErrorCode != ERROR_SUCCESS) {
            ShowErrorBox(lErrorCode);
            return 1;
        }

        DWORD waitResult = WaitForSingleObject(hEvent, INFINITE);
        if (waitResult == WAIT_OBJECT_0) {
            CheckRegistry(hKey);
        }
    }

    RegCloseKey(hKey);
    CloseHandle(hEvent);
}
