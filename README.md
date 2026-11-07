# Panel Tabs Redux

Tabs for the left and right panels of FAR Manager 3.0 (build 6666 or newer). It is a rewrite of the FarTabs plugin.

- Each panel has its own list of tabs, saved between sessions.
- A tab follows you as you browse, or you can pin it to a fixed directory.
- New tabs get a shortcut key (1–9, then A–Z) for quick switching.
- Works in archives and in plugin panels that support FAR folder shortcuts, such as FTP or network folders.
- English and Russian interface. It follows FAR's language setting.

The tab list opens in a menu (the "picker"). FarTabs showed tabs in an always-visible strip, which this version does not have.

## Install

1. Download the ZIP matching your **FAR.exe** (`x86` or `x64`) from the GitHub Releases page.
2. Unpack it into `%FARPROFILE%\Plugins\PanelTabsRedux\`, or into a subfolder of FAR's `Plugins` directory. To see your profile path, run `echo %FARPROFILE%` from FAR's command line.
3. Optional: for hotkeys, copy `PanelTabs.lua` into `%FARPROFILE%\Macros\scripts\`, then run `macro:load` in FAR.
4. Restart FAR.

To build from source, see [BUILDING.md](BUILDING.md).

## Use

To open the picker, choose **Panel Tabs Redux** from **Alt+F1** (left panel) or **Alt+F2** (right panel). You can also open it for the active panel from **F11** or with `ptabs:`. If the entry is missing from Alt+F1/Alt+F2, press **F9** there and enable **Show plugins**. Press **F1** in the picker or the tab editor for help.

### In the picker

| Key | Action |
| --- | --- |
| **Enter** or the tab's shortcut | Switch to the tab |
| **Insert** | Add a tab for the current directory |
| **F4** | Edit the tab's name, path, shortcut, and pin state |
| **F3** | Pin or unpin the tab |
| **Delete** | Close the tab |
| **Ctrl+Up / Ctrl+Down** | Move the tab |
| **Tab** | Show the other panel's tabs |

An unpinned tab follows the directory you browse to. A pinned tab keeps its saved directory, and selecting it takes you back there.

Some tabs are marked **[Open only]**. These point to something that cannot be reopened later, such as a nested archive or a Temporary Panel. They work only while that panel is still open.

### Hotkeys

These hotkeys need `PanelTabs.lua` and work when FAR's command line is empty.

| Hotkey | Action |
| --- | --- |
| **Ctrl+Alt+Space** | Open the picker |
| **Ctrl+Alt+T** | New tab for the current directory |
| **Ctrl+Alt+W** | Close the current tab |
| **Ctrl+Alt+PgUp / PgDn** | Previous / next tab |
| **Alt+Shift+1–9 / A–Z** | Switch to the tab with that shortcut |

To change the keys, edit `PanelTabs.lua`. The same actions are available as `ptabs:new`, `ptabs:close`, `ptabs:prev`, `ptabs:next`, and `ptabs:<shortcut>`, such as `ptabs:1`.

Plugin folder shortcuts can contain credentials. For that reason, saved tabs are encrypted for your Windows account.

## Updates through Renewal

Copy `RenewalCustom.xml` from the ZIP next to `Renewal.dll`. If a `RenewalCustom.xml` is already there, add the Panel Tabs Redux `<mod>` entry to its `<pd>` section instead. Then check for updates in Renewal. If you set up Panel Tabs Redux in Renewal before, enable the custom source for its entry.

Renewal does not update the macro script. If you use `PanelTabs.lua`, copy the new version yourself.

## Translations

To add a language, copy `PanelTabsReduxEng.lng` and change its `.Language` header. Then translate the quoted strings. For the help, copy `PanelTabsReduxEng.hlf` the same way and translate the text, keeping the `@Topic` lines and link targets unchanged.

- Keep the order of the messages.
- Keep the `%s` placeholder in the macro description.
- Save the files as UTF-8 with a BOM, next to the DLL.

After changing FAR's language, run `macro:load` to update the macro descriptions.
