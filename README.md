# Panel Tabs Redux

Panel Tabs Redux is a C++ reimplementation of the Delphi FarTabs plugin for FAR Manager 3.0 build 6666 or newer. The left and right filesystem panels each have their own list of tabs. An unpinned tab follows the directory as you browse. Tabs remember the cursor position when you switch away during the session. Tab commands save directory paths, custom names, and pin states to FAR's profile.

The picker uses FAR's native menu and input boxes. The always-visible strip in the Delphi plugin uses a console overlay, so this version puts the tab list in the picker. This version handles filesystem directories. Archive panels, other plugin panels, and Delphi settings are outside its scope.

## Use

Choose **Panel Tabs Redux** from **Alt+F1** for the left panel or **Alt+F2** for the right panel, regardless of which panel has focus. If the entry is hidden, press **F9** in the drive selector and enable **Show plugins**. **F11 → Panel Tabs Redux** and `ptabs:` open the picker for the active panel.

In the picker, press **Insert** to add a directory as a tab, **F4** to edit the highlighted tab's directory, **F2** to rename it, or **Delete** to close it. **Enter** and **1–9** select tabs, and **Tab** shows the other visible filesystem panel's list. The checkmark identifies the current tab. A new picker entry stays inactive until you select it. Editing the current tab also changes the panel's directory.

Press **F3** in the picker to pin or unpin the highlighted tab. A pinned tab keeps its saved directory while you browse. Select it again to return to that directory. **F4** can still change the saved directory. If you unpin the current tab, it adopts the directory you are browsing. New and existing tabs are unpinned by default.

With the supplied LuaMacro script, these keys work from an empty FAR command line.

| Hotkey | Action |
| --- | --- |
| **Ctrl+Alt+Space** | Open the tab picker |
| **Ctrl+Alt+T** | Open a tab in the active panel's current directory |
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
4. Restart FAR and open the picker from **Alt+F1**, **Alt+F2**, or **F11**.

FAR sets `FARPROFILE` inside its session. To see the active profile path, run `echo %FARPROFILE%` from FAR's command line. The DLL uses a separate plugin ID and the `ptabs:` prefix.
