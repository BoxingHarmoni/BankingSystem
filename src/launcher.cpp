#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <filesystem>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Get executable path
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path binDir = std::filesystem::path(exePath).parent_path();
    std::filesystem::path rootDir = binDir.parent_path();
    std::filesystem::path htmlPath = rootDir / "web" / "index.html";

    if (!std::filesystem::exists(htmlPath)) {
        htmlPath = binDir / "web" / "index.html";
    }

    std::wstring fileUri = L"file:///" + htmlPath.wstring();
    for (size_t i = 0; i < fileUri.length(); ++i) {
        if (fileUri[i] == L'\\') fileUri[i] = L'/';
    }

    // Attempt to launch via msedge in standalone App mode
    std::wstring appArg = L"--app=\"" + fileUri + L"\" --window-size=1200,820";

    HINSTANCE hInst = ShellExecuteW(
        nullptr,
        L"open",
        L"msedge.exe",
        appArg.c_str(),
        nullptr,
        SW_SHOWNORMAL
    );

    // If msedge wasn't found as a direct command, try Chrome or fallback to shell default
    if ((INT_PTR)hInst <= 32) {
        hInst = ShellExecuteW(
            nullptr,
            L"open",
            L"chrome.exe",
            appArg.c_str(),
            nullptr,
            SW_SHOWNORMAL
        );
    }

    if ((INT_PTR)hInst <= 32) {
        // Fallback: open directly with default system browser or native Win32 window
        ShellExecuteW(nullptr, L"open", fileUri.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    return 0;
}
