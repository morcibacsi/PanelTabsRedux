#include <plugin.hpp>
#include <objbase.h>
#include <wincrypt.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <new>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
constexpr GUID PluginId{0x88ff2c99, 0xfaac, 0x494f, {0x8b, 0x40, 0xb5, 0xb5, 0xf2, 0x74, 0xed, 0xfc}};
constexpr GUID MenuId{0x5121d451, 0x1fa2, 0x418e, {0x9c, 0xf3, 0xe2, 0xec, 0x1e, 0x94, 0x27, 0xfd}};
constexpr GUID DiskMenuId{0xbb777a2e, 0xeb7c, 0x4278, {0xbb, 0x7a, 0x60, 0x9e, 0x21, 0x3e, 0x7f, 0xf4}};
constexpr GUID RenameId{0x3bba88e6, 0x1048, 0x4832, {0x9c, 0x54, 0x3b, 0xc7, 0xdf, 0xfc, 0x7d, 0x7c}};
constexpr GUID EditPathId{0x52df1904, 0x42ad, 0x431e, {0xa1, 0x57, 0x82, 0x3a, 0x28, 0x7f, 0xa7, 0x61}};
constexpr GUID ErrorId{0x821e68d3, 0x31ea, 0x4901, {0x8b, 0xc0, 0x41, 0x70, 0xf2, 0x2a, 0x23, 0xd8}};
constexpr auto Title = L"Panel Tabs Redux";
constexpr auto SettingsKey = L"TabListsV4";
constexpr std::size_t TabLimit = 999;

struct Cursor
{
    std::wstring file;
    std::size_t index{};
    std::size_t offset{};
};

struct Location
{
    std::wstring path;
    GUID plugin = FarGuid;
    std::wstring file;
    std::wstring param;
    HANDLE livePanel{};

    bool IsPlugin() const { return plugin != FarGuid; }
    bool HasLocalHostFile() const { return IsPlugin() && std::filesystem::path(file).is_absolute(); }
    bool Valid() const { return IsPlugin() || !path.empty(); }
    bool SamePluginPanel(const Location& other) const
    {
        return IsPlugin() && plugin == other.plugin && livePanel == other.livePanel &&
               (livePanel || (file == other.file && param == other.param));
    }
    bool operator==(const Location&) const = default;
};

bool HasTemporaryHostFile(const Location& location)
{
    if (!location.HasLocalHostFile())
        return false;

    // FAR extracts nested archives into directories named FAR<hex>.tmp under the Windows temp path.
    wchar_t tempPath[MAX_PATH + 1]{};
    const auto length = GetTempPathW(static_cast<DWORD>(std::size(tempPath)), tempPath);
    if (!length || length >= std::size(tempPath))
        return false;
    const auto host = std::filesystem::path(location.file).lexically_normal().make_preferred().native();
    const auto prefix = std::filesystem::path(tempPath).lexically_normal().make_preferred().native();
    if (host.size() <= prefix.size() ||
        CompareStringOrdinal(host.data(), static_cast<int>(prefix.size()), prefix.data(),
                             static_cast<int>(prefix.size()), TRUE) != CSTR_EQUAL)
        return false;

    const std::wstring_view relative(host.data() + prefix.size(), host.size() - prefix.size());
    const auto slash = relative.find(L'\\');
    if (slash == relative.npos)
        return false;
    const auto folder = relative.substr(0, slash);
    if (folder.size() < 8 || folder.size() > 15 ||
        CompareStringOrdinal(folder.data(), 3, L"FAR", 3, TRUE) != CSTR_EQUAL ||
        CompareStringOrdinal(folder.data() + folder.size() - 4, 4, L".tmp", 4, TRUE) != CSTR_EQUAL)
        return false;
    return folder.substr(3, folder.size() - 7).find_first_not_of(L"0123456789abcdefABCDEF") == folder.npos;
}

bool NeedsLivePanel(const Location& location)
{
    return location.livePanel || HasTemporaryHostFile(location);
}

bool CanOpenLocation(const Location& current, const Location& target)
{
    return !NeedsLivePanel(target) || target.SamePluginPanel(current);
}

std::wstring LocationLabel(const Location& location)
{
    if (!location.IsPlugin())
        return location.path;
    const auto host = location.HasLocalHostFile()
        ? (HasTemporaryHostFile(location) ? std::filesystem::path(location.file).filename().wstring() : location.file)
        : L"Plugin panel";
    return location.path.empty() ? host : host + L" :: " + location.path;
}

struct Tab
{
    Location location;
    std::wstring name;
    Cursor cursor;
    bool pinned{};
};

struct TabList
{
    std::vector<Tab> tabs;
    std::size_t active{};
    bool attached{};
};

PluginStartupInfo api{};
std::array<TabList, 2> lists;
bool dirty{};
bool canSave = true;

void Warn(const wchar_t* message, const wchar_t* detail = nullptr)
{
    const wchar_t* items[]{Title, message, detail};
    api.Message(&PluginId, &ErrorId, FMSG_WARNING | FMSG_MB_OK, nullptr, items, detail ? 3 : 2, 0);
}

class Settings
{
public:
    Settings()
    {
        FarSettingsCreate request{sizeof(request), PluginId, INVALID_HANDLE_VALUE};
        if (api.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, PSL_LOCAL, &request))
            handle = request.Handle;
    }

    ~Settings()
    {
        if (handle != INVALID_HANDLE_VALUE)
            api.SettingsControl(handle, SCTL_FREE, 0, nullptr);
    }

    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    HANDLE handle = INVALID_HANDLE_VALUE;
};

struct LocalFreeBuffer
{
    void operator()(BYTE* data) const { LocalFree(data); }
};

std::optional<std::wstring> DecryptSettings(const FarSettingsItem& item)
{
    if (!item.Data.Data || !item.Data.Size || item.Data.Size > MAXDWORD)
        return std::nullopt;
    DATA_BLOB input{static_cast<DWORD>(item.Data.Size),
                    const_cast<BYTE*>(static_cast<const BYTE*>(item.Data.Data))};
    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
        return std::nullopt;
    const std::unique_ptr<BYTE, LocalFreeBuffer> bytes(output.pbData);
    if (!output.cbData || output.cbData % sizeof(wchar_t))
        return std::nullopt;
    std::wstring result(output.cbData / sizeof(wchar_t), L'\0');
    std::memcpy(result.data(), bytes.get(), output.cbData);
    return result;
}

void Load()
{
    const Settings settings;
    if (settings.handle == INVALID_HANDLE_VALUE)
        return;

    FarSettingsItem item{};
    item.StructSize = sizeof(item);
    item.Name = SettingsKey;
    item.Type = FST_DATA;
    if (!api.SettingsControl(settings.handle, SCTL_GET, 0, &item))
        return;

    auto data = DecryptSettings(item);
    if (!data)
    {
        canSave = false;
        Warn(L"FAR could not decrypt the saved tab lists for this Windows account.");
        return;
    }

    std::wistringstream input(std::move(*data));
    std::array<TabList, 2> loaded;
    for (auto& list : loaded)
    {
        std::size_t count{};
        if (!(input >> count >> list.active) || count > TabLimit ||
            (count ? list.active >= count : list.active != 0))
            return;

        list.tabs.resize(count);
        for (auto& tab : list.tabs)
        {
            auto& location = tab.location;
            std::wstring guid;
            if (!(input >> std::quoted(location.path) >> std::quoted(tab.name) >> tab.pinned
                        >> std::quoted(guid) >> std::quoted(location.file) >> std::quoted(location.param)) ||
                FAILED(IIDFromString(guid.c_str(), &location.plugin)) || !location.Valid())
                return;
        }
    }
    if (input >> std::ws && input.eof())
        lists = std::move(loaded);
}

void Save()
{
    if (!dirty || !canSave)
        return;

    std::wostringstream output;
    for (const auto& list : lists)
    {
        std::size_t count{};
        std::size_t active{};
        std::wostringstream entries;
        for (std::size_t i = 0; i < list.tabs.size(); ++i)
        {
            const auto& tab = list.tabs[i];
            const auto& location = tab.location;
            if (NeedsLivePanel(location))
                continue;
            ++count;
            if (i < list.active)
                ++active;
            wchar_t guid[39]{};
            StringFromGUID2(location.plugin, guid, static_cast<int>(std::size(guid)));
            entries << std::quoted(location.path) << L' ' << std::quoted(tab.name) << L' ' << tab.pinned
                    << L' ' << std::quoted(guid) << L' ' << std::quoted(location.file)
                    << L' ' << std::quoted(location.param) << L'\n';
        }
        output << count << L' ' << (count ? std::min(active, count - 1) : 0) << L'\n' << entries.str();
    }

    // FAR shortcut data is opaque and may contain credentials.
    auto data = output.str();
    if (data.size() > MAXDWORD / sizeof(wchar_t))
    {
        Warn(L"The tab lists are too large to save.");
        return;
    }
    DATA_BLOB plaintext{static_cast<DWORD>(data.size() * sizeof(wchar_t)), reinterpret_cast<BYTE*>(data.data())};
    DATA_BLOB encrypted{};
    if (!CryptProtectData(&plaintext, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &encrypted))
    {
        Warn(L"Windows could not encrypt the tab lists.");
        return;
    }
    const std::unique_ptr<BYTE, LocalFreeBuffer> bytes(encrypted.pbData);
    const Settings settings;
    FarSettingsItem item{};
    item.StructSize = sizeof(item);
    item.Name = SettingsKey;
    item.Type = FST_DATA;
    item.Data = {encrypted.cbData, bytes.get()};
    if (settings.handle != INVALID_HANDLE_VALUE &&
        api.SettingsControl(settings.handle, SCTL_SET, 0, &item))
        dirty = false;
    else
        Warn(L"FAR could not save the tab lists to this profile.");
}

PanelInfo FilePanel(HANDLE panel)
{
    PanelInfo info{};
    info.StructSize = sizeof(info);
    if (!api.PanelControl(panel, FCTL_GETPANELINFO, 0, &info) ||
        info.PanelType != PTYPE_FILEPANEL ||
        !(info.Flags & PFLAGS_VISIBLE))
        return {};
    return info;
}

// FAR fills a structure and its trailing strings in one caller-owned buffer.
class PanelBuffer
{
public:
    void* Get(std::size_t bytes)
    {
        if (bytes > capacity)
        {
            data = std::make_unique_for_overwrite<std::byte[]>(bytes);
            capacity = bytes;
        }
        return data.get();
    }

private:
    std::unique_ptr<std::byte[]> data;
    std::size_t capacity{};
};

Location ReadLocation(HANDLE panel, const PanelInfo& info)
{
    if (!info.StructSize)
        return {};
    const auto bytes = api.PanelControl(panel, FCTL_GETPANELDIRECTORY, 0, nullptr);
    if (bytes < static_cast<intptr_t>(sizeof(FarPanelDirectory)))
        return {};
    PanelBuffer buffer;
    auto* directory = ::new (buffer.Get(static_cast<std::size_t>(bytes))) FarPanelDirectory{};
    directory->StructSize = sizeof(*directory);
    const auto written = api.PanelControl(panel, FCTL_GETPANELDIRECTORY, bytes, directory);
    if (written <= 0 || written > bytes)
        return {};
    Location location{
        directory->Name ? directory->Name : L"",
        directory->PluginId,
        directory->File ? directory->File : L"",
        directory->Param ? directory->Param : L""
    };
    if (location.IsPlugin() && !(info.Flags & PFLAGS_SHORTCUT))
        location.livePanel = info.PluginHandle;
    return location;
}

Location CurrentLocation(HANDLE panel)
{
    return ReadLocation(panel, FilePanel(panel));
}

std::wstring_view ItemName(HANDLE panel, std::size_t index, PanelBuffer& buffer)
{
    const auto bytes = api.PanelControl(panel, FCTL_GETPANELITEM, index, nullptr);
    if (bytes < static_cast<intptr_t>(sizeof(PluginPanelItem)))
        return {};
    auto* item = ::new (buffer.Get(static_cast<std::size_t>(bytes))) PluginPanelItem{};
    FarGetPluginPanelItem request{sizeof(request), static_cast<std::size_t>(bytes), item};
    const auto written = api.PanelControl(panel, FCTL_GETPANELITEM, index, &request);
    return written > 0 && written <= bytes && item->FileName ? item->FileName : L"";
}

Cursor ReadCursor(HANDLE panel, const PanelInfo& info)
{
    PanelBuffer buffer;
    return {
        info.CurrentItem < info.ItemsNumber ? std::wstring(ItemName(panel, info.CurrentItem, buffer)) : L"",
        info.CurrentItem,
        info.CurrentItem - std::min(info.CurrentItem, info.TopPanelItem)
    };
}

void RestoreCursor(HANDLE panel, const Cursor& cursor)
{
    if (cursor.file.empty())
        return;
    const auto info = FilePanel(panel);
    PanelBuffer buffer;
    auto index = cursor.index;
    if (index >= info.ItemsNumber || ItemName(panel, index, buffer) != cursor.file)
    {
        // Sort order or directory contents may have changed since the last visit.
        for (index = 0; index < info.ItemsNumber; ++index)
            if (ItemName(panel, index, buffer) == cursor.file)
                break;
    }
    if (index < info.ItemsNumber)
    {
        PanelRedrawInfo redraw{sizeof(redraw), index, index - std::min(index, cursor.offset)};
        api.PanelControl(panel, FCTL_REDRAWPANEL, 0, &redraw);
    }
}

TabList* Sync(HANDLE panel)
{
    const auto info = FilePanel(panel);
    if (!info.StructSize)
        return nullptr;
    auto location = ReadLocation(panel, info);
    if (!location.Valid())
        return nullptr;

    auto& list = lists[(info.Flags & PFLAGS_PANELLEFT) ? 0 : 1];
    if (NeedsLivePanel(location) && !list.tabs.empty() &&
        !list.tabs[list.active].location.SamePluginPanel(location))
    {
        // Browsing a session-only panel must not replace an existing saved destination.
        list.attached = true;
        return &list;
    }

    // FAR chooses the initial directories. Preserve saved tabs when it starts elsewhere.
    if (!list.attached && (list.tabs.empty() || list.tabs[list.active].location != location))
    {
        const auto match = std::ranges::find(list.tabs, location, &Tab::location);
        if (match != list.tabs.end())
        {
            list.active = static_cast<std::size_t>(match - list.tabs.begin());
            dirty = true;
        }
        else if (list.tabs.empty() || !list.tabs[list.active].pinned)
        {
            if (list.tabs.size() == TabLimit)
            {
                Warn(L"The panel's tab list is full. Open FAR in a saved tab's directory to manage it.");
                return nullptr;
            }
            list.tabs.push_back({location, {}, {}});
            list.active = list.tabs.size() - 1;
            dirty = true;
        }
    }
    list.attached = true;

    auto& tab = list.tabs[list.active];
    if (tab.pinned && tab.location != location)
        return &list;
    if (tab.location != location)
    {
        tab.location = std::move(location);
        dirty = true;
    }
    tab.cursor = ReadCursor(panel, info);
    return &list;
}

bool CanAdd(const TabList& list)
{
    if (list.tabs.size() < TabLimit)
        return true;
    Warn(L"This panel has reached its tab limit.");
    return false;
}

void Add(HANDLE panel, TabList& list)
{
    if (!CanAdd(list))
        return;
    auto location = CurrentLocation(panel);
    if (!location.Valid())
        return;
    list.tabs.push_back({std::move(location), {}, ReadCursor(panel, FilePanel(panel))});
    list.active = list.tabs.size() - 1;
    dirty = true;
}

bool OpenLocation(HANDLE panel, const Location& location)
{
    FarPanelDirectory target{sizeof(target), location.path.c_str(), location.param.c_str(),
                             location.plugin, location.file.c_str()};
    return api.PanelControl(panel, FCTL_SETPANELDIRECTORY, 0, &target) != 0;
}

bool GoToLocation(HANDLE panel, TabList& list, const Location& current, const Location& target)
{
    if (!CanOpenLocation(current, target))
    {
        Warn(L"Open this location on the panel before selecting its tab.",
             LocationLabel(target).c_str());
        return false;
    }
    // A panel without a shortcut can restore its cursor only. SetDirectory can close it.
    if (target.livePanel)
        return true;

    const auto cursor = ReadCursor(panel, FilePanel(panel));
    if (OpenLocation(panel, target))
        return true;

    // FAR can change the panel while trying to open a plugin shortcut.
    const auto after = CurrentLocation(panel);
    if (after != current)
    {
        if (!CanOpenLocation(after, current) || (!current.livePanel && !OpenLocation(panel, current)))
        {
            list.attached = false;
            Warn(L"Cannot open the tab or restore the previous location.", LocationLabel(target).c_str());
            return false;
        }
        RestoreCursor(panel, cursor);
    }
    Warn(L"Cannot open the tab's location.", LocationLabel(target).c_str());
    return false;
}

bool SwitchTo(HANDLE panel, TabList& list, std::size_t index)
{
    const auto currentLocation = CurrentLocation(panel);
    const auto& tab = list.tabs[index];
    if (index == list.active && currentLocation == tab.location)
        return true;
    if (tab.location != currentLocation && !GoToLocation(panel, list, currentLocation, tab.location))
        return false;
    RestoreCursor(panel, tab.cursor);
    if (list.active != index)
    {
        list.active = index;
        dirty = true;
    }
    return true;
}

std::optional<std::size_t> AdjacentTab(HANDLE panel, const TabList& list, bool forward)
{
    const auto current = CurrentLocation(panel);
    const auto count = list.tabs.size();
    const auto step = forward ? 1 : count - 1;
    auto index = list.active;
    for (std::size_t offset = 1; offset < count; ++offset)
    {
        index = (index + step) % count;
        if (CanOpenLocation(current, list.tabs[index].location))
            return index;
    }
    return std::nullopt;
}

void Close(HANDLE panel, TabList& list, std::size_t index)
{
    if (list.tabs.size() == 1)
    {
        Warn(L"Keep at least one tab on each panel.");
        return;
    }
    if (index == list.active)
    {
        const auto replacement = AdjacentTab(panel, list, index + 1 < list.tabs.size());
        if (!replacement)
        {
            Warn(L"Open another tab before closing this one.");
            return;
        }
        if (!SwitchTo(panel, list, *replacement))
            return;
    }
    list.tabs.erase(list.tabs.begin() + static_cast<std::ptrdiff_t>(index));
    if (index < list.active)
        --list.active;
    dirty = true;
}

void Rename(Tab& tab)
{
    wchar_t name[128]{};
    if (api.InputBox(&PluginId, &RenameId, L"Rename tab", L"Leave blank to use the directory path",
                     nullptr, tab.name.c_str(), name, std::size(name), nullptr,
                     FIB_BUTTONS | FIB_ENABLEEMPTY) && tab.name != name)
    {
        tab.name = name;
        dirty = true;
    }
}

std::optional<std::wstring> PromptForDirectory(const wchar_t* title, const Location& location)
{
    const auto plugin = location.IsPlugin();
    const auto prompt = plugin ? L"Directory on the plugin panel" : L"Absolute directory path";
    std::wstring path(32768, L'\0');
    if (!api.InputBox(&PluginId, &EditPathId, title, prompt,
                      nullptr, location.path.c_str(), path.data(), path.size(), nullptr,
                      FIB_BUTTONS | (plugin ? FIB_ENABLEEMPTY : FIB_EDITPATH)))
        return std::nullopt;

    path.resize(std::char_traits<wchar_t>::length(path.c_str()));
    if (!plugin && !std::filesystem::path(path).is_absolute())
    {
        Warn(L"Enter an absolute directory path.");
        return std::nullopt;
    }
    return path;
}

void EditPath(HANDLE panel, TabList& list, std::size_t index)
{
    auto& tab = list.tabs[index];
    if (tab.location.livePanel)
    {
        Warn(L"This panel does not provide a folder shortcut.");
        return;
    }
    auto path = PromptForDirectory(L"Edit tab directory", tab.location);
    if (!path || *path == tab.location.path)
        return;
    auto target = tab.location;
    target.path = std::move(*path);
    if (index == list.active)
    {
        const auto current = CurrentLocation(panel);
        if (target != current && !GoToLocation(panel, list, current, target))
            return;
    }

    tab.location = std::move(target);
    tab.cursor = {};
    dirty = true;
}

void AppendMenuText(std::wstring& line, std::wstring_view text)
{
    for (const auto ch : text)
    {
        if (ch == L'&')
            line += L'&';
        line += ch < L' ' ? L' ' : ch;
    }
}

void Pick(HANDLE panel, TabList* list)
{
    constexpr FarKey keys[]{{VK_INSERT, 0}, {VK_DELETE, 0}, {VK_F2, 0}, {VK_TAB, 0},
                            {VK_F4, 0}, {VK_F3, 0}, {}};
    auto highlight = list->active;
    for (;;)
    {
        std::vector<std::wstring> labels(list->tabs.size());
        std::vector<FarMenuItem> items(list->tabs.size());
        for (std::size_t i = 0; i < items.size(); ++i)
        {
            const auto& tab = list->tabs[i];
            auto& label = labels[i];
            if (i < 9)
                label += L'&';
            label += std::to_wstring(i + 1) + L"  ";
            if (tab.pinned)
                label += L"[Pinned] ";
            if (NeedsLivePanel(tab.location))
                label += L"[Open only] ";
            if (!tab.name.empty())
            {
                AppendMenuText(label, tab.name);
                label += L"  |  ";
            }
            AppendMenuText(label, LocationLabel(tab.location));
            items[i].Text = label.c_str();
            items[i].Flags = (i == list->active ? MIF_CHECKED : MIF_NONE) |
                             (i == highlight ? MIF_SELECTED : MIF_NONE);
        }

        const bool left = list == &lists[0];
        intptr_t pressed = -1;
        const auto result = api.Menu(&PluginId, &MenuId, -1, -1, 0, FMENU_WRAPMODE,
                                     left ? L"Left panel tabs" : L"Right panel tabs",
                                     L"Ins New  Del Close  F2 Rename  F3 Pin  F4 Edit path  Tab Other panel",
                                     nullptr, keys, &pressed, items.data(), items.size());
        if (result < 0 || static_cast<std::size_t>(result) >= items.size())
            return;
        highlight = static_cast<std::size_t>(result);
        switch (pressed)
        {
        case 0:
            if (CanAdd(*list))
            {
                auto location = CurrentLocation(panel);
                if (location.livePanel)
                    location = {};
                if (!location.IsPlugin())
                {
                    auto path = PromptForDirectory(L"New tab", location);
                    if (!path)
                        break;
                    location.path = std::move(*path);
                }
                list->tabs.push_back({std::move(location), {}, {}});
                highlight = list->tabs.size() - 1;
                dirty = true;
            }
            break;
        case 1:
            Close(panel, *list, highlight);
            highlight = std::min(highlight, list->tabs.size() - 1);
            break;
        case 2:
            Rename(list->tabs[highlight]);
            break;
        case 3:
        {
            const auto other = panel == PANEL_ACTIVE ? PANEL_PASSIVE : PANEL_ACTIVE;
            if (auto* otherList = Sync(other))
            {
                panel = other;
                list = otherList;
                highlight = list->active;
            }
            else
                Warn(L"The other panel must be a visible file or plugin panel.");
            break;
        }
        case 4:
            EditPath(panel, *list, highlight);
            break;
        case 5:
        {
            auto& tab = list->tabs[highlight];
            tab.pinned = !tab.pinned;
            dirty = true;
            if (!tab.pinned && highlight == list->active)
                Sync(panel);
            break;
        }
        default:
            if (SwitchTo(panel, *list, highlight))
                return;
            break;
        }
    }
}

void Dispatch(std::wstring_view command, HANDLE panel)
{
    constexpr auto whitespace = L" \t\r\n";
    const auto first = command.find_first_not_of(whitespace);
    command = first == command.npos
        ? std::wstring_view{} : command.substr(first, command.find_last_not_of(whitespace) - first + 1);
    const bool number = command.size() == 1 && command.front() >= L'1' && command.front() <= L'9';
    if (!command.empty() && command != L"list" && command != L"new" &&
        command != L"close" && command != L"next" && command != L"prev" && !number)
    {
        Warn(L"Use ptabs: with list, new, close, next, prev, or a tab number from 1 to 9.");
        return;
    }

    auto* list = Sync(panel);
    if (!list)
    {
        Warn(L"Panel tabs need a visible file or plugin panel.");
        return;
    }
    const auto count = list->tabs.size();
    if (command.empty() || command == L"list")
        Pick(panel, list);
    else if (command == L"new")
        Add(panel, *list);
    else if (command == L"close")
        Close(panel, *list, list->active);
    else if (command == L"next" || command == L"prev")
        SwitchTo(panel, *list, AdjacentTab(panel, *list, command == L"next").value_or(list->active));
    else
    {
        const auto index = static_cast<std::size_t>(command.front() - L'1');
        if (index < count)
            SwitchTo(panel, *list, index);
        else
            MessageBeep(MB_OK);
    }
    Save();
}
}

void WINAPI GetGlobalInfoW(GlobalInfo* info)
{
    info->StructSize = sizeof(*info);
    info->MinFarVersion = MAKEFARVERSION(3, 0, 0, 6666, VS_RELEASE);
    info->Version = MAKEFARVERSION(
        PANELTABS_VERSION_MAJOR, PANELTABS_VERSION_MINOR, PANELTABS_VERSION_PATCH, 0, VS_RELEASE);
    info->Guid = PluginId;
    info->Title = Title;
    info->Description = L"Keyboard-driven tabs for FAR file and plugin panels";
    info->Author = L"Panel Tabs Redux contributors";
}

void WINAPI GetPluginInfoW(PluginInfo* info)
{
    static const wchar_t* const menu[]{Title};
    info->StructSize = sizeof(*info);
    info->Flags = PF_NONE;
    info->DiskMenu = {&DiskMenuId, menu, std::size(menu)};
    info->PluginMenu = {&MenuId, menu, std::size(menu)};
    info->CommandPrefix = L"ptabs";
}

void WINAPI SetStartupInfoW(const PluginStartupInfo* info)
{
    api = *info;
    try
    {
        Load();
    }
    catch (...)
    {
        Warn(L"FAR could not read the saved tab lists.");
    }
}

HANDLE WINAPI OpenW(const OpenInfo* info)
{
    try
    {
        const wchar_t* command = L"";
        auto panel = PANEL_ACTIVE;
        if (info->OpenFrom == OPEN_COMMANDLINE)
        {
            const auto* args = reinterpret_cast<const OpenCommandLineInfo*>(info->Data);
            if (args && args->CommandLine)
                command = args->CommandLine;
        }
        else if (info->OpenFrom == OPEN_FROMMACRO)
        {
            const auto* args = reinterpret_cast<const OpenMacroInfo*>(info->Data);
            if (args && args->Count && args->Values[0].Type == FMVT_STRING && args->Values[0].String)
                command = args->Values[0].String;
        }
        else if (info->OpenFrom == OPEN_LEFTDISKMENU || info->OpenFrom == OPEN_RIGHTDISKMENU)
        {
            PanelInfo active{};
            active.StructSize = sizeof(active);
            if (!api.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &active))
                return nullptr;

            const bool activeIsLeft = (active.Flags & PFLAGS_PANELLEFT) != 0;
            const bool targetIsLeft = info->OpenFrom == OPEN_LEFTDISKMENU;
            panel = activeIsLeft == targetIsLeft ? PANEL_ACTIVE : PANEL_PASSIVE;
        }
        else if (info->OpenFrom != OPEN_PLUGINSMENU)
            return nullptr;

        Dispatch(command, panel);
    }
    catch (...)
    {
        Warn(L"Could not complete the tab operation.");
    }
    return nullptr;
}
