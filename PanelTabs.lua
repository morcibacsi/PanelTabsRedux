local plugin = "88FF2C99-FAAC-494F-8B40-B5B5F274EDFC"
local messagePlugin = win.Uuid(plugin)

-- The first six entries in each Panel Tabs Redux language file.
local function description(id, fallback)
  local message = far.GetMsg(id, messagePlugin)
  return message and message ~= "" and message or fallback
end

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

bind("CtrlAltSpace", "list", description(0, "Pick a tab"))
bind("CtrlAltT", "new", description(1, "Open a tab"))
bind("CtrlAltW", "close", description(2, "Close the current tab"))
bind("CtrlAltPgUp", "prev", description(3, "Previous tab"))
bind("CtrlAltPgDn", "next", description(4, "Next tab"))

local goToTab = description(5, "Go to tab %s")
for shortcut in ("123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"):gmatch(".") do
  bind("AltShift" .. shortcut, shortcut, goToTab:format(shortcut), true)
end
