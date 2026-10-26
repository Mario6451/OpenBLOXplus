# OpenBLOX Lua Script Engine

## Overview
Provides Roblox-like Lua scripting support for OpenBLOX. Engine can run without Lua (fallback mode).

## Supported Script Types
- **.lua** — Standard Lua 5.4 (default)
- **.luau** — Luau dialect (Roblox's custom Lua)
- **.lua51** — Lua 5.1 (legacy Roblox scripts)

## Luau Compatibility
The engine automatically converts Luau-specific syntax to Lua 5.4:

| Luau Feature | Conversion |
|---|---|
| `local x: number = 10` | `local x = 10` (type annotations removed) |
| `continue` | `break` |
| `x += 5` | `x = x + 5` |
| `x -= 2` | `x = x - 2` |
| `1_000_000` | `1000000` (numeric literals) |

## Deprecated Roblox APIs
The following deprecated Roblox APIs are available for compatibility:

| API | Description |
|---|---|
| `tick()` | Returns Unix timestamp |
| `elapsedTime()` | Seconds since game started |
| `version()` | Engine version string |
| `typeof(x)` | Returns type name as string |
| `delay(n, func)` | Call func after n seconds |
| `spawn(func)` | Call func asynchronously |
| `require(name)` | Load ModuleScript |
| `game:service(name)` | Legacy GetService alias |
| `math.mod(a, b)` | Legacy modulo (fmod) |

## Building with Lua Support

1. Download Lua 5.4 source from https://www.lua.org/
2. Extract to `C_Engine/ThirdParty/lua/`
3. Add `-DHAVE_LUA` to CXXFLAGS in makefile_3ds
4. Add `-llua` to LDFLAGS
5. Add `luau_compat.cpp` to SOURCES in makefile_3ds

Without HAVE_LUA, the engine will compile and run but scripts will be skipped.

## API Reference

### Core Functions
- `ScriptEngine::init()` - Initialize Lua state (includes deprecated APIs)
- `ScriptEngine::shutdown()` - Cleanup and close Lua
- `ScriptEngine::loadScript(path)` - Load and execute a .lua/.luau file (auto-detects version)
- `ScriptEngine::loadScriptsFromDirectory(path)` - Scan directory for .lua and .luau files
- `ScriptEngine::loadScriptsFromNode(node)` - Find Script child nodes and load
- `ScriptEngine::getLoadedScriptCount()` - Get number of loaded scripts

### Luau Version Detection
- `ScriptEngine::detectLuaVersion(path)` - Detect version from file extension
- `ScriptEngine::detectLuaVersionFromSource(source)` - Detect from source content
- `ScriptEngine::convertLuauToLua(source)` - Convert Luau to Lua 5.4
- `ScriptEngine::preprocessScript(source, version)` - Auto-preprocess based on version

### Roblox API Bindings
- `Instance.new(className)` - Create new Node instance
- `game:GetService(name)` - Access game services (RunService, Players, etc.)
- `game:service(name)` - Legacy alias (deprecated)
- `Vector3.new(x, y, z)` - Create vector3
- `CFrame.new(x, y, z)` - Create coordinate frame
- `Color3.new(r, g, b)` - Create color (0-1 range)
- `BrickColor.new(number)` - Create brick color from palette index

### Instance Methods
- `:FindFirstChild(name)` - Find child by name
- `:findFirstChild(name)` - Legacy alias (deprecated)
- `:GetName()` - Get instance name
- `:GetParent()` - Get parent instance

## Script Examples

```lua
-- Standard Lua 5.4 script (.lua)
print("Hello from OpenBLOX!")
local workspace = game:GetService("Workspace")
local part = Instance.new("Part")
part.Name = "TestPart"
wait(1)
print("Done")
```

```lua
-- Luau script with type annotations (.luau)
print("Luau script!")
local count: number = 0
count += 5
for i = 1, 10 do
    if i % 2 == 0 then continue end
    count += i
end
print(count)
```

## Notes
- Scripts run in coroutines for isolation
- Engine supports legacy Roblox Lua API (pre-2016)
- Full Lua 5.4 standard library available
- Luau type annotations are stripped at load time (no runtime type checking)
- Both .lua and .luau files are loaded from directories
