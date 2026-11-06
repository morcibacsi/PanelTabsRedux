local plugin = "88FF2C99-FAAC-494F-8B40-B5B5F274EDFC"

local function bind(key, command, description, tabShortcut)
  Macro {
    area = "Shell";
    key = key;
    flags = "EmptyCommandLine EnableOutput";
    priority = tabShortcut and 40 or 50;
    description = "Panel Tabs Redux: " .. description;
    condition = function()
      return APanel.Visible and Plugin.Exist(plugin)
        and (not tabShortcut or Plugin.SyncCall(plugin, "has-shortcut", command) == true)
    end;
    action = function()
      Plugin.Call(plugin, command)
    end;
  }
end

bind("CtrlAltSpace", "list", "Pick a tab")
bind("CtrlAltT", "new", "Open a tab")
bind("CtrlAltW", "close", "Close the current tab")
bind("CtrlAltPgUp", "prev", "Previous tab")
bind("CtrlAltPgDn", "next", "Next tab")

for shortcut in ("123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"):gmatch(".") do
  bind("AltShift" .. shortcut, shortcut, "Go to tab " .. shortcut, true)
end
