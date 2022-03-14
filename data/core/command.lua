local core = require "core"
local command = {}

command.map = {}

local always_true = function() return true end


function command.add(predicate, map)
  predicate = predicate or always_true
  -- 如果是 string，那么 require 指定的文件
  if type(predicate) == "string" then
    predicate = require(predicate)
  end
  -- 这个 table 类型不太清楚
  if type(predicate) == "table" then
    local class = predicate
    predicate = function() return core.active_view:is(class) end
  end

  -- predicate 到这里都会是一个函数
  for name, fn in pairs(map) do
    -- 确认 command 没有被添加过
    assert(not command.map[name], "command already exists: " .. name)
    -- 放到 command.map 中
    command.map[name] = { predicate = predicate, perform = fn }
  end
end


local function capitalize_first(str)
  return str:sub(1, 1):upper() .. str:sub(2)
end

function command.prettify_name(name)
  return name:gsub(":", ": "):gsub("-", " "):gsub("%S+", capitalize_first)
end


function command.get_all_valid()
  local res = {}
  for name, cmd in pairs(command.map) do
    if cmd.predicate() then
      table.insert(res, name)
    end
  end
  return res
end


local function perform(name)
  local cmd = command.map[name]
  if cmd and cmd.predicate() then
    cmd.perform()
    return true
  end
  return false
end


function command.perform(...)
  local ok, res = core.try(perform, ...)
  return not ok or res
end


function command.add_defaults()
  -- 载入默认的命令
  -- reg 里的就是 commands.* 里的命令，也就是默认的命令
  local reg = { "core", "root", "command", "doc", "findreplace" }
  for _, name in ipairs(reg) do
    require("core.commands." .. name)
  end
end


return command
