# Panel Tabs Redux

Panel Tabs Redux is a C++ reimplementation of the Delphi FarTabs plugin for FAR Manager 3.0 build 6666 or newer. The left and right filesystem panels each have their own list of tabs. An unpinned tab follows the directory as you browse. Tabs remember the cursor position when you switch away during the session. Tab commands save directories, names, pin states, shortcuts, and order to FAR's profile.

The picker uses FAR's native menu and input boxes. The always-visible strip in the Delphi plugin uses a console overlay, so this version puts the tab list in the picker. Tabs can save filesystem folders and locations exposed through FAR folder shortcuts, including archives and remote folders.

## Use

Choose **Panel Tabs Redux** from **Alt+F1** for the left panel or **Alt+F2** for the right panel, regardless of which panel has focus. If the entry is hidden, press **F9** in the drive selector and enable **Show plugins**. **F11 → Panel Tabs Redux** and `ptabs:` open the picker for the active panel.

In the picker, press **Insert** to add a tab, **F4** to edit it, or **Delete** to close it. **Enter** or an assigned shortcut selects a tab. **Tab** shows the other visible filesystem panel's list. The checkmark identifies the current tab. A new picker entry stays inactive until you select it.

**F4** opens an editor for the tab's name, path, shortcut, and pin state. Changing the current tab's path also changes the panel's directory. New tabs get the first available shortcut from **1–9**, then **A–Z**. Clear the shortcut field to leave the tab unassigned. If a shortcut is already in use on that panel, **Use anyway** transfers it to the edited tab. **Cancel** returns to the editor with the draft intact. Press **Ctrl+Up** or **Ctrl+Down** in the picker to move the highlighted tab while keeping its shortcut.

Press **F3** in the picker to pin or unpin the highlighted tab. A pinned tab keeps its saved directory while you browse. Select it again to return to that directory. **F4** can still change the saved directory. If you unpin the current tab, it adopts the directory you are browsing. New tabs are unpinned by default.

From inside a supported archive, open the picker or use the usual tab shortcuts. **Insert** adds a tab for the current archive folder. **F4** edits the folder inside the archive, with a blank path for the archive root. An unpinned tab follows you into and out of an archive just as it follows ordinary folders.

FAR extracts nested archives to temporary files. Tabs created in these archives are marked **[Open only]** and can jump between folders while that archive is displayed on the panel. Browsing a temporary archive preserves your existing saved destination. Once the temporary archive closes, reopen it from the original archive to browse it again.

The picker and hotkeys also work on visible plugin file panels. A panel that provides a FAR folder shortcut can be saved and reopened through that shortcut. FAR can reuse the live panel if the shortcut identifies the same connection. Saved tab data is encrypted for the current Windows account because plugin shortcuts can contain credentials.

Panels without folder shortcuts, such as Temporary Panel, can still use the picker. **Insert** adds a filesystem destination from those panels. Tabs that point to a live panel are marked **[Open only]** and can restore the cursor while that panel is open. Use **Tab** in the picker to switch tabs on the other side and keep the live panel open.

With the supplied LuaMacro script, these keys work from an empty FAR command line.

| Hotkey | Action |
| --- | --- |
| **Ctrl+Alt+Space** | Open the tab picker |
| **Ctrl+Alt+T** | Open a tab in the active panel's current directory |
| **Ctrl+Alt+W** | Close the current tab |
| **Ctrl+Alt+PgUp / PgDn** | Previous / next tab |
| **Alt+Shift+1–9 / A–Z** | Switch to the tab with that shortcut |

Edit `PanelTabs.lua` to rebind the keys. The same commands are available through `ptabs:new`, `ptabs:close`, `ptabs:prev`, `ptabs:next`, and a shortcut such as `ptabs:1` or `ptabs:A`.

## Build and install

Use Visual Studio 2026 with the Desktop development with C++ workload and CMake 4.2 or newer. Keep the FAR Manager source checkout in a sibling directory named `FarManager` for the plugin SDK header. Open this folder in Visual Studio as a CMake project, or run the following commands from this folder.

```powershell
cmake -S . -B out/x86 -G "Visual Studio 18 2026" -A Win32
cmake --build out/x86 --config Release

cmake -S . -B out/x64 -G "Visual Studio 18 2026" -A x64
cmake --build out/x64 --config Release
```

1. Choose the DLL that matches the architecture of **FAR.exe**. The build writes it to `out/x86/bin/Release/PanelTabsRedux.dll` or `out/x64/bin/Release/PanelTabsRedux.dll`.
2. Copy the DLL into `%FARPROFILE%\Plugins\PanelTabsRedux\` or a subfolder of FAR's main `Plugins` directory. If you are upgrading, remove the installed `PanelTabsCpp.dll` when you install `PanelTabsRedux.dll`.
3. For hotkeys, copy `PanelTabs.lua` into `%FARPROFILE%\Macros\scripts\`. If LuaMacro uses a custom script path, use that path instead. Run `macro:load` in FAR after editing the script.
4. Restart FAR and open the picker from **Alt+F1**, **Alt+F2**, or **F11**.

FAR sets `FARPROFILE` inside its session. To see the active profile path, run `echo %FARPROFILE%` from FAR's command line. The DLL uses a separate plugin ID and the `ptabs:` prefix.

## Updates through Renewal

1. Copy `RenewalCustom.xml` beside `Renewal.dll`. If that directory already has a `RenewalCustom.xml`, add the Panel Tabs Redux `<mod>` entry to its `<pd>` section.
2. Open Renewal and check for updates. If you previously configured Panel Tabs Redux in Renewal, enable the custom source for its entry.

Renewal uses the latest stable GitHub release and downloads the ZIP matching FAR's architecture. If you installed the optional LuaMacro script, update the copy in your macro directory separately.

### Build release ZIPs

Push a version tag such as `v1.0.1` to GitHub. The release workflow builds both architectures with the version from the tag and attaches the ZIPs to a draft GitHub release. Publish the draft as a stable release to make it available to Renewal.

To build release ZIPs locally, pass `-DPANELTABS_VERSION=1.0.1` when configuring each architecture, build both Release configurations, then run these commands from the project directory.

```powershell
cpack --config out/x86/CPackConfig.cmake -C Release -B out/packages
cpack --config out/x64/CPackConfig.cmake -C Release -B out/packages
```

CMake uses the same version for the ZIP filenames and for the plugin version that Renewal reads from the DLL. Publish a higher version for Renewal to offer an update.
