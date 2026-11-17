## Features
- **Automatic Fullscreen Detection:** Detects both exclusive fullscreen and borderless fullscreen windows while ignoring normal maximized desktop apps.
- **Anti-Cheat Safe:** Uses standard Win32 API calls (`ClipCursor`, `GetForegroundWindow`) with zero DLL injection or memory reading.
- **Instant Release:** Automatically unlocks your cursor the moment you `Alt+Tab`, press the `Windows` key, or minimize the game.
- **Global Hotkey:** Press `F9` (or left-click the system tray icon) to toggle the lock on or off at any time.
- **Lightweight:** Written in native C++ with no external dependencies

## Usage
1. Launch `CursorLock.exe` (it starts minimized in the Windows system tray).
2. Open any fullscreen game, your cursor will automatically stay confined to that monitor.
3. Press **`F9`** or left-click the tray icon to pause/resume the lock, or right-click the tray icon to **Exit**.

### Run at Windows Startup (Optional)
1. Press `Win + R`, type `shell:startup`, and press `Enter`.
2. Place a shortcut to `CursorLock.exe` inside the Startup folder.

## Building from Source
1. Open the solution in **Visual Studio 2022** (or newer) with the **Desktop development with C++** workload installed.
2. Set the C++ Language Standard to **ISO C++17** (`/std:c++17`) or newer.
3. Ensure the Linker SubSystem is set to **Windows (`/SUBSYSTEM:WINDOWS`)**.
4. Select **Release | x64** and build (`Ctrl + Shift + B`).
