#include "CursorLock.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

void LoadExclusions()
{
    g_excludedProc = EXCLUDED_PROC;

    std::string txtPath = GetExcludePath();
    if (txtPath.empty())
        return;

    std::ifstream file(txtPath);
    if (!file.is_open())
        return;

    std::string line;
    while (std::getline(file, line))
    {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) 
            continue;
        size_t end = line.find_last_not_of(" \t\r\n");
        std::string trimmed = line.substr(start, end - start + 1);

        if (trimmed.empty() || trimmed[0] == '#' || (trimmed.size() >= 2 && trimmed[0] == '/' && trimmed[1] == '/'))
            continue;

        g_excludedProc.push_back(trimmed);
    }
}

std::string GetExcludePath()
{
    char basePath[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, basePath)))    
        return std::string(basePath) + "\\" + APP + "\\" + EXCLUDE_FILE;
    
    return "";
}

void OpenExclude()
{
    std::string txtPath = GetExcludePath();
    if (txtPath.empty())
        return;

    size_t lastSlash = txtPath.find_last_of("\\/");
    if (lastSlash != std::string::npos)
        CreateDirectoryA(txtPath.substr(0, lastSlash).c_str(), NULL);

    std::ifstream checkFile(txtPath);
    if (!checkFile.is_open())
    {
        std::ofstream newFile(txtPath);
        if (newFile.is_open())
        {
            newFile << "# Add one .exe name per line to exclude from CursorLock (case-insensitive)\n";
            newFile << "# Example:\n";
            newFile << "# chrome.exe\n";
        }
    }
    else    
        checkFile.close();    

    ShellExecuteA(NULL, "open", txtPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
}


std::string GetProcName(HWND hwnd)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid) return "";

    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) return "";

    char fullPath[MAX_PATH] = { 0 };
    DWORD size = MAX_PATH;
    std::string exeName = "";
    if (QueryFullProcessImageNameA(hProc, 0, fullPath, &size))
    {
        std::string pathStr(fullPath);
        size_t pos = pathStr.find_last_of("\\/");
        exeName = (pos != std::string::npos) ? pathStr.substr(pos + 1) : pathStr;
    }
    CloseHandle(hProc);
    return exeName;
}

bool IsFullscreen(RECT& outMonitorRect)
{
    HWND fgHwnd = GetForegroundWindow();
    if (!fgHwnd || !IsWindowVisible(fgHwnd) || IsIconic(fgHwnd))
        return false;

    char className[256] = {0};
    GetClassNameA(fgHwnd, className, sizeof(className));
    std::string cls(className);
    if (cls == "WorkerW" || cls == "Progman" || cls == "Shell_TrayWnd" || cls == "Shell_SecondaryTrayWnd")
        return false;

    std::string procName = GetProcName(fgHwnd);
    for (const auto& excluded : g_excludedProc)    
        if (_stricmp(procName.c_str(), excluded.c_str()) == 0)
            return false;    

    LONG_PTR style = GetWindowLongPtrA(fgHwnd, GWL_STYLE);
    if ((style & WS_CAPTION) == WS_CAPTION || (style & WS_THICKFRAME) != 0)
        return false;

    HMONITOR hMon = MonitorFromWindow(fgHwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoA(hMon, &mi))
        return false;

    RECT win{};
    if (!GetWindowRect(fgHwnd, &win))
        return false;

    if (win.left <= mi.rcMonitor.left && win.top <= mi.rcMonitor.top && win.right >= mi.rcMonitor.right && win.bottom >= mi.rcMonitor.bottom)
    {
        outMonitorRect = mi.rcMonitor;
        return true;
    }

    return false;
}


void UpdateText(HWND hwnd)
{
    std::string tip = g_isLocked ? "CursorLock: ENABLED (Press F9 to pause)" : "CursorLock: PAUSED (Press F9 to resume)";
    strcpy_s(g_nid.szTip, tip.c_str());
    Shell_NotifyIconA(NIM_MODIFY, &g_nid);
}

void ToggleLockState(HWND hwnd)
{
    g_isLocked = !g_isLocked;
    if (!g_isLocked && g_isClipped)
    {
        ClipCursor(NULL);
        g_isClipped = false;
    }
    Beep(g_isLocked ? 800 : 400, 100);
    UpdateText(hwnd);
}

void ShowMenu(HWND hwnd)
{
    POINT pt{};
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    AppendMenuA(hMenu, MF_STRING | (g_isLocked ? MF_CHECKED : MF_UNCHECKED), ID_TOGGLE, "Enable Fullscreen Cursor Lock (F9)");
    AppendMenuA(hMenu, MF_STRING, ID_RELOAD, "Reload exclude.txt");
    AppendMenuA(hMenu, MF_STRING, ID_OPEN_CFG, "Edit exclude.txt");
    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMenu, MF_STRING, ID_EXIT, "Exit");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        RegisterHotKey(hwnd, ID_HOTKEY_F9, 0, HOTKEY);
        SetTimer(hwnd, ID_TIMER, POLL_INTERVAL_MS, NULL);
        break;

    case WM_HOTKEY:
        if (wParam == ID_HOTKEY_F9)        
            ToggleLockState(hwnd);        
        break;

    case WM_TIMER:
        if (wParam == ID_TIMER)
        {
            RECT monRect{};
            if (g_isLocked && IsFullscreen(monRect))
            {
                ClipCursor(&monRect);
                g_isClipped = true;
            }
            else if (g_isClipped)
            {
                ClipCursor(NULL);
                g_isClipped = false;
            }
        }
        break;

    case WM_ICON:
        if (lParam == WM_LBUTTONUP)        
            ToggleLockState(hwnd);        
        else if (lParam == WM_RBUTTONUP)        
            ShowMenu(hwnd);        
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TOGGLE)        
            ToggleLockState(hwnd);        
        else if (LOWORD(wParam) == ID_RELOAD)        
            LoadExclusions();        
        else if (LOWORD(wParam) == ID_EXIT)        
            DestroyWindow(hwnd);
        else if (LOWORD(wParam) == ID_OPEN_CFG)        
            OpenExclude();        
        break;

    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER);
        UnregisterHotKey(hwnd, ID_HOTKEY_F9);
        if (g_isClipped) 
            ClipCursor(NULL);
        Shell_NotifyIconA(NIM_DELETE, &g_nid);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    LoadExclusions();
    HANDLE hMutex = CreateMutexA(NULL, TRUE, MUTEX);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        if (hMutex)
            CloseHandle(hMutex);
        return 0;
    }

    WNDCLASSA wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WIN_CLASS;
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(0, WIN_CLASS, APP, 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_ICON;
    g_nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
    strcpy_s(g_nid.szTip, "CursorLock: ENABLED (Press F9 to pause)");
    Shell_NotifyIconA(NIM_ADD, &g_nid);

    MSG msg{};
    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    if (hMutex) 
        CloseHandle(hMutex);
    return 0;
}
