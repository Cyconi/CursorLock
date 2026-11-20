#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <shlobj.h>
#include "resource.h"

constexpr UINT WM_ICON = WM_USER + 1;
constexpr UINT ID_TOGGLE = 1001;
constexpr UINT ID_RELOAD = 1002;
constexpr UINT ID_OPEN_CFG = 1003;
constexpr UINT ID_EXIT = 1004;
constexpr UINT ID_TIMER = 2001;
constexpr UINT ID_HOTKEY_F9 = 3001;

constexpr UINT POLL_INTERVAL_MS = 15;
constexpr UINT HOTKEY = VK_F9;
constexpr const char* MUTEX = "Global\\CursorLockSingleInstanceMutex";
constexpr const char* WIN_CLASS = "CursorLockHiddenWindow";
constexpr const char* APP = "CursorLock";
constexpr const char* EXCLUDE_FILE = "exclude.txt";

inline const std::vector<std::string> EXCLUDED_PROC = 
{
    "ScreenClippingHost.exe",
    "SnippingTool.exe",
    "ShareX.exe",
    "Greenshot.exe",
    "Lightshot.exe",
    "RobloxPlayerBeta.exe"
};

inline NOTIFYICONDATAA g_nid{};
inline bool g_isLocked = true;
inline bool g_isClipped = false;
inline std::vector<std::string> g_excludedProc = EXCLUDED_PROC;

void LoadExclusions();
std::string GetExcludePath();
void OpenExclude();
std::string GetProcName(HWND hwnd);
bool IsFullscreen(RECT& outMonitorRect);
void UpdateText(HWND hwnd);
void ToggleLockState(HWND hwnd);
void ShowMenu(HWND hwnd);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
