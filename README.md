# Panel Tabs for FAR Manager

Panel Tabs is a C++ plugin for FAR Manager 3.0 build 6666 or newer. The left and right filesystem panels each have their own list of tabs. A tab follows the directory as you browse and remembers the cursor position when you switch away during the session. Tab commands save directory paths and custom names to FAR's profile.

The picker is a native FAR menu, and renaming uses FAR's native input box. The public API provides these transient controls for the built-in panels. The always-visible strip in the Delphi plugin uses a console overlay, so this version puts the tab list in the picker. This version handles filesystem directories. Archive panels, other plugin panels, and Delphi settings are outside its scope.

## Use

Open **F11 → Panel Tabs (C++)** or run `ptabs:` at the FAR command line. In the picker, use **Enter** or **1–9** to switch, **Insert** to open a new tab, **Delete** to close the highlighted tab, **F2** to rename it, or **Tab** to show the other visible filesystem panel's tabs. The checkmark identifies the current tab. Opening a tab starts in the current directory so you can browse elsewhere while keeping the original tab.

With the supplied LuaMacro script, these keys work from an empty FAR command line.

| Hotkey | Action |
| --- | --- |
| **Ctrl+Alt+Space** | Open the tab picker |
| **Ctrl+Alt+T** | Open a tab on the active panel |
| **Ctrl+Alt+W** | Close the current tab |
| **Ctrl+Alt+PgUp / PgDn** | Previous / next tab |
| **Alt+Shift+1–9** | Switch to a numbered tab |

Edit `PanelTabs.lua` to rebind the keys. The same commands are available through `ptabs:new`, `ptabs:close`, `ptabs:prev`, `ptabs:next`, and `ptabs:1` through `ptabs:9`.

## Build and install

Use Visual Studio 2026 with the Desktop development with C++ workload and CMake 4.2 or newer. Open this folder in Visual Studio as a CMake project, or run the following commands from this folder.

```powershell
cmake -S . -B out/x86 -G "Visual Studio 18 2026" -A Win32
cmake --build out/x86 --config Release

cmake -S . -B out/x64 -G "Visual Studio 18 2026" -A x64
cmake --build out/x64 --config Release
```

1. Choose the DLL that matches the architecture of **FAR.exe**. The build writes it to `out/x86/bin/Release/PanelTabsCpp.dll` or `out/x64/bin/Release/PanelTabsCpp.dll`.
2. Copy the DLL into `%FARPROFILE%\Plugins\PanelTabsCpp\` or a subfolder of FAR's main `Plugins` directory.
3. For hotkeys, copy `PanelTabs.lua` into `%FARPROFILE%\Macros\scripts\`. If LuaMacro uses a custom script path, use that path instead. Run `macro:load` in FAR after editing the script.
4. Restart FAR and open the picker from **F11**.

FAR sets `FARPROFILE` inside its session. To see the active profile path, run `echo %FARPROFILE%` from FAR's command line. The DLL uses a separate plugin ID and the `ptabs:` prefix.
