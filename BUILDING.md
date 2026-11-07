# Building Panel Tabs Redux

## Requirements

- Visual Studio 2026 with the Desktop development with C++ workload
- CMake 4.2 or newer
- The FAR Manager source checkout in a sibling directory named `FarManager`, for the plugin SDK header

## Build

Open this folder in Visual Studio as a CMake project, or run the following commands from this folder.

```powershell
cmake -S . -B out/x86 -G "Visual Studio 18 2026" -A Win32
cmake --build out/x86 --config Release

cmake -S . -B out/x64 -G "Visual Studio 18 2026" -A x64
cmake --build out/x64 --config Release
```

The DLL is written to `out/x86/bin/Release/PanelTabsRedux.dll` or `out/x64/bin/Release/PanelTabsRedux.dll`. To install it, copy the DLL and both `.lng` files into a `PanelTabsRedux` folder under FAR's `Plugins` directory. Use the DLL that matches the architecture of **FAR.exe**.

If you are upgrading from an early build, remove the old `PanelTabsCpp.dll`.

## Releases

Push a version tag such as `v1.0.1` to GitHub. The release workflow builds both architectures with the version from the tag and attaches the ZIPs to a draft GitHub release. Publish the draft as a stable release to make it available to Renewal. Renewal offers an update only when the published version is higher than the installed one.

To build the release ZIPs locally, pass `-DPANELTABS_VERSION=1.0.1` when configuring each architecture and build both Release configurations. Then run:

```powershell
cpack --config out/x86/CPackConfig.cmake -C Release -B out/packages
cpack --config out/x64/CPackConfig.cmake -C Release -B out/packages
```

The ZIP filenames and the plugin version that Renewal reads from the DLL both use this version.
