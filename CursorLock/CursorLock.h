#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include "resource.h"

constexpr UINT WM_TRAYICON = WM_USER + 1;
constexpr UINT ID_TRAY_TOGGLE = 1001;
constexpr UINT ID_TRAY_RELOAD = 1002;
constexpr UINT ID_TRAY_EXIT = 1003;
constexpr UINT ID_TIMER_CHECK = 2001;
constexpr UINT ID_HOTKEY_F9 = 3001;

constexpr UINT POLL_INTERVAL_MS = 15;
constexpr UINT HOTKEY_KEY = VK_F9;
constexpr const char* MUTEX_NAME = "Global\\CursorLockSingleInstanceMutex";
constexpr const char* WINDOW_CLASS_NAME = "CursorLockHiddenWindow";
constexpr const char* APP_TITLE = "CursorLock";
constexpr const char* EXCLUDE_FILE_NAME = "exclude.txt";

inline const std::vector<std::string> EXCLUDED_PROCESSES = 
{
    "ScreenClippingHost.exe",
    "SnippingTool.exe",
    "ShareX.exe",
    "Greenshot.exe",
    "Lightshot.exe",
    "RobloxPlayerBeta.exe"
};

inline NOTIFYICONDATAA g_nid{};
inline bool g_lockEnabled = true;
inline bool g_isClipped = false;
inline std::vector<std::string> g_excludedProcesses = EXCLUDED_PROCESSES;

void LoadCustomExclusions();
std::string GetWindowProcessName(HWND hwnd);
bool IsForegroundWindowFullscreen(RECT& outMonitorRect);
void UpdateTrayTooltip(HWND hwnd);
void ToggleLockState(HWND hwnd);
void ShowTrayMenu(HWND hwnd);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
