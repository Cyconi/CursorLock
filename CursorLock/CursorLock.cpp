#include "CursorLock.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

// Checks if the active foreground window is in true Fullscreen (ignores normal maximized windows)
bool IsForegroundWindowFullscreen(RECT& outMonitorRect)
{
    HWND fgHwnd = GetForegroundWindow();
    if (!fgHwnd || !IsWindowVisible(fgHwnd) || IsIconic(fgHwnd))
        return false;

    // Ignore the Windows desktop wallpaper and taskbar
    char className[256] = {0};
    GetClassNameA(fgHwnd, className, sizeof(className));
    std::string cls(className);
    if (cls == "WorkerW" || cls == "Progman" || cls == "Shell_TrayWnd" || cls == "Shell_SecondaryTrayWnd")
        return false;

    // Check window styles: normal maximized apps have a title bar (WS_CAPTION) or resize border (WS_THICKFRAME).
    // True fullscreen games (Exclusive or Borderless) strip these styles away.
    LONG_PTR style = GetWindowLongPtrA(fgHwnd, GWL_STYLE);
    if ((style & WS_CAPTION) == WS_CAPTION || (style & WS_THICKFRAME) != 0)
        return false;

    // Get the exact monitor the active window is running on
    HMONITOR hMon = MonitorFromWindow(fgHwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoA(hMon, &mi))
        return false;

    // Get the active window's screen coordinates
    RECT win{};
    if (!GetWindowRect(fgHwnd, &win))
        return false;

    // Verify the window covers the entire monitor (including over the taskbar area)
    if ((win.left <= mi.rcMonitor.left && win.top <= mi.rcMonitor.top && win.right >= mi.rcMonitor.right && win.bottom >= mi.rcMonitor.bottom))
    {
        outMonitorRect = mi.rcMonitor;
        return true;
    }

    return false;
}

void UpdateTrayTooltip(HWND hwnd)
{
    std::string tip = g_lockEnabled ? "CursorLock: ENABLED (Press F9 to pause)" : "CursorLock: PAUSED (Press F9 to resume)";
    strcpy_s(g_nid.szTip, tip.c_str());
    Shell_NotifyIconA(NIM_MODIFY, &g_nid);
}

void ToggleLockState(HWND hwnd)
{
    g_lockEnabled = !g_lockEnabled;
    if (!g_lockEnabled && g_isClipped)
    {
        ClipCursor(NULL);
        g_isClipped = false;
    }
    Beep(g_lockEnabled ? 800 : 400, 100);
    UpdateTrayTooltip(hwnd);
}

void ShowTrayMenu(HWND hwnd)
{
    POINT pt{};
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    AppendMenuA(hMenu, MF_STRING | (g_lockEnabled ? MF_CHECKED : MF_UNCHECKED),
        ID_TRAY_TOGGLE, "Enable Fullscreen Cursor Lock (F9)");
    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMenu, MF_STRING, ID_TRAY_EXIT, "Exit");

    // Required by Windows so the menu closes properly when clicking outside it
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        RegisterHotKey(hwnd, ID_HOTKEY_F9, 0, HOTKEY_KEY);
        SetTimer(hwnd, ID_TIMER_CHECK, POLL_INTERVAL_MS, NULL);
        break;

    case WM_HOTKEY:
        if (wParam == ID_HOTKEY_F9)        
            ToggleLockState(hwnd);        
        break;

    case WM_TIMER:
        if (wParam == ID_TIMER_CHECK)
        {
            RECT monRect{};
            if (g_lockEnabled && IsForegroundWindowFullscreen(monRect))
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

    case WM_TRAYICON:
        if (lParam == WM_LBUTTONUP)        
            ToggleLockState(hwnd);        
        else if (lParam == WM_RBUTTONUP)        
            ShowTrayMenu(hwnd);        
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TRAY_TOGGLE)        
            ToggleLockState(hwnd);        
        else if (LOWORD(wParam) == ID_TRAY_EXIT)        
            DestroyWindow(hwnd);        
        break;

    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER_CHECK);
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
    HANDLE hMutex = CreateMutexA(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        if (hMutex)
            CloseHandle(hMutex);
        return 0;
    }

    WNDCLASSA wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    RegisterClassA(&wc);

    // Create a hidden message-only window (no taskbar button or visible window)
    HWND hwnd = CreateWindowExA(0, WINDOW_CLASS_NAME, APP_TITLE, 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    // Add icon to the Windows system tray
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL)); // Uses small.ico for a crisp 16x16 tray icon!

    //g_nid.hIcon = LoadCursor(NULL, IDC_ARROW); // standard Windows mouse cursor icon
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
