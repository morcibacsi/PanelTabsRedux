local plugin = "88FF2C99-FAAC-494F-8B40-B5B5F274EDFC"

local function bind(key, command, description)
  Macro {
    area = "Shell";
    key = key;
    flags = "EmptyCommandLine EnableOutput NoPluginPanels";
    description = "Panel Tabs Redux: " .. description;
    condition = function()
      return APanel.Visible and Plugin.Exist(plugin)
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

for index = 1, 9 do
  bind("AltShift" .. index, tostring(index), "Go to tab " .. index)
end
