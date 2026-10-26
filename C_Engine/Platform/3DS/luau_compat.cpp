#include "script_engine.h"
#include "CoreEngine/Node/nodes.h"
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <thread>

extern Node *dataModel;

namespace ScriptEngine {

// ============================================================================
// Lua/Luau Version Detection
// ============================================================================

LuaVersion detectLuaVersion(const std::string& filePath) {
    size_t pos = filePath.find_last_of('.');
    if (pos == std::string::npos) return LuaVersion::Lua54;
    std::string ext = filePath.substr(pos);
    if (ext == ".luau" || ext == ".Luau" || ext == ".LUAU")
        return LuaVersion::Luau;
    if (ext == ".lua51" || ext == ".Lua51")
        return LuaVersion::Lua51;
    return LuaVersion::Lua54;  // .lua defaults to Lua 5.4
}

LuaVersion detectLuaVersionFromSource(const std::string& source) {
    if (source.find("#!luau") != std::string::npos)
        return LuaVersion::Luau;
    if (source.find("#!lua51") != std::string::npos)
        return LuaVersion::Lua51;
    if (source.find(": number") != std::string::npos ||
        source.find(": string") != std::string::npos ||
        source.find(": boolean") != std::string::npos ||
        source.find(": table") != std::string::npos ||
        source.find(": any") != std::string::npos) {
        return LuaVersion::Luau;
    }
    if (source.find("continue") != std::string::npos) {
        return LuaVersion::Luau;
    }
    return LuaVersion::Lua54;
}

// ============================================================================
// Luau to Lua 5.4 Converter
// ============================================================================

std::string convertLuauToLua(const std::string& luauSource) {
    std::string result;
    result.reserve(luauSource.size() + 512);
    std::string line;
    const char* src = luauSource.c_str();
    size_t len = luauSource.size();
    size_t i = 0;

    while (i < len) {
        while (i < len && src[i] != '\n' && src[i] != '\r')
            line += src[i++];
        if (i < len) {
            if (src[i] == '\r') i++;
            if (i < len && src[i] == '\n') { line += '\n'; i++; }
        }

        std::string convertedLine;
        bool inString = false;
        char stringChar = 0;
        size_t j = 0;

        while (j < line.size()) {
            char c = line[j];

            if (!inString && (c == '"' || c == '\'')) {
                inString = true; stringChar = c;
                convertedLine += c; j++; continue;
            }
            if (inString) {
                convertedLine += c;
                if (c == stringChar) {
                    if (j + 1 < line.size() && line[j+1] == stringChar)
                        { convertedLine += line[j+1]; j += 2; }
                    else { inString = false; j++; }
                } else j++;
                continue;
            }
            if (c == '-' && j + 1 < line.size() && line[j+1] == '-') {
                convertedLine += line.substr(j); break;
            }

            // Type annotations: `: type` -> remove
            if (c == ':') {
                size_t k = j + 1;
                while (k < line.size() && (line[k] == ' ' || line[k] == '\t')) k++;
                if (k < line.size() &&
                    ((line[k] >= 'a' && line[k] <= 'z') ||
                     (line[k] >= 'A' && line[k] <= 'Z') ||
                     line[k] == '|' || line[k] == '{' || line[k] == '?' ||
                     line[k] == '$' || line[k] == '[')) {
                    size_t endType = k;
                    while (endType < line.size() && line[endType] != '=' &&
                           line[endType] != ',' && line[endType] != '\n') {
                        if (line[endType] == '(') {
                            int p = 1; endType++;
                            while (endType < line.size() && p > 0) {
                                if (line[endType]=='(') p++;
                                if (line[endType]==')') p--;
                                endType++;
                            }
                            continue;
                        }
                        if (line[endType] == '{') {
                            int b = 1; endType++;
                            while (endType < line.size() && b > 0) {
                                if (line[endType]=='{') b++;
                                if (line[endType]=='}') b--;
                                endType++;
                            }
                            continue;
                        }
                        endType++;
                    }
                    if (endType < line.size() && line[endType] == '=') {
                        convertedLine += line.substr(endType); j = line.size();
                    } else if (endType < line.size() && line[endType] == ',') {
                        convertedLine += ','; j = endType + 1;
                    } else j = line.size();
                    continue;
                }
            }

            // continue -> break
            if (c == 'c' && line.substr(j, 8) == "continue") {
                convertedLine += "break"; j += 8; continue;
            }

            // Compound assignments: += -= *= /=
            if (c == '+' && j+1 < line.size() && line[j+1] == '=') {
                size_t k2 = j - 1;
                while (k2 > 0 && (line[k2-1] == ' ' || line[k2-1] == '\t')) k2--;
                std::string var;
                while (k2 > 0 && ((line[k2-1]>='a'&&line[k2-1]<='z')||(line[k2-1]>='A'&&line[k2-1]<='Z')||(line[k2-1]>='0'&&line[k2-1]<='9')||line[k2-1]=='_')) k2--;
                var = line.substr(k2, j - k2);
                convertedLine += var + " = " + var + " + "; j += 2; continue;
            }
            if (c == '-' && j+1 < line.size() && line[j+1] == '=') {
                size_t k2 = j - 1;
                while (k2 > 0 && (line[k2-1] == ' ' || line[k2-1] == '\t')) k2--;
                std::string var;
                while (k2 > 0 && ((line[k2-1]>='a'&&line[k2-1]<='z')||(line[k2-1]>='A'&&line[k2-1]<='Z')||(line[k2-1]>='0'&&line[k2-1]<='9')||line[k2-1]=='_')) k2--;
                var = line.substr(k2, j - k2);
                convertedLine += var + " = " + var + " - "; j += 2; continue;
            }
            if (c == '*' && j+1 < line.size() && line[j+1] == '=') {
                size_t k2 = j - 1;
                while (k2 > 0 && (line[k2-1] == ' ' || line[k2-1] == '\t')) k2--;
                std::string var;
                while (k2 > 0 && ((line[k2-1]>='a'&&line[k2-1]<='z')||(line[k2-1]>='A'&&line[k2-1]<='Z')||(line[k2-1]>='0'&&line[k2-1]<='9')||line[k2-1]=='_')) k2--;
                var = line.substr(k2, j - k2);
                convertedLine += var + " = " + var + " * "; j += 2; continue;
            }
            if (c == '/' && j+1 < line.size() && line[j+1] == '=') {
                size_t k2 = j - 1;
                while (k2 > 0 && (line[k2-1] == ' ' || line[k2-1] == '\t')) k2--;
                std::string var;
                while (k2 > 0 && ((line[k2-1]>='a'&&line[k2-1]<='z')||(line[k2-1]>='A'&&line[k2-1]<='Z')||(line[k2-1]>='0'&&line[k2-1]<='9')||line[k2-1]=='_')) k2--;
                var = line.substr(k2, j - k2);
                convertedLine += var + " = " + var + " / "; j += 2; continue;
            }

            // Numeric literals with underscores: 1_000_000 -> 1000000
            if (c >= '0' && c <= '9') {
                std::string num;
                while (j < line.size() && ((line[j]>='0'&&line[j]<='9')||line[j]=='_'||line[j]=='.')) {
                    if (line[j] != '_') num += line[j];
                    j++;
                }
                convertedLine += num; continue;
            }

            convertedLine += c; j++;
        }
        result += convertedLine;
        line.clear();
    }
    return result;
}

std::string convertLua51ToLua54(const std::string& lua51Source) {
    // Most Lua 5.1 features work in 5.4; math.mod -> math.fmod handled via wrapper
    return lua51Source;
}

std::string preprocessScript(const std::string& source, LuaVersion version) {
    switch (version) {
        case LuaVersion::Luau:  return convertLuauToLua(source);
        case LuaVersion::Lua51: return convertLua51ToLua54(source);
        default: return source;
    }
}

// ============================================================================
// Deprecated Roblox API Functions
// ============================================================================

void setupDeprecatedAPIs() {
#if defined(HAVE_LUA)
    if (!L) return;

    // math.mod -> math.fmod
    lua_getglobal(L, "math");
    {
        lua_pushcfunction(L, [](lua_State* L) -> int {
            double a = lua_tonumber(L, 1), b = lua_tonumber(L, 2);
            lua_pushnumber(L, fmod(a, b)); return 1;
        });
        lua_setfield(L, -2, "mod");
    }
    lua_pop(L, 1);

    // tick() -> unix timestamp
    lua_pushcfunction(L, [](lua_State* L) -> int {
        lua_pushnumber(L, (double)std::chrono::duration<double, std::chrono::seconds::period>::count(
            std::chrono::system_clock::now().time_since_epoch()));
        return 1;
    });
    lua_setglobal(L, "tick");

    // elapsedTime()
    lua_pushcfunction(L, [](lua_State* L) -> int {
        static auto start = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        lua_pushnumber(L, elapsed); return 1;
    });
    lua_setglobal(L, "elapsedTime");

    // version()
    lua_pushcfunction(L, [](lua_State* L) -> int {
        lua_pushstring(L, "OpenBLOX 0.1.0 (Lua 5.4 + Luau compat)"); return 1;
    });
    lua_setglobal(L, "version");

    // warn()
    lua_pushcfunction(L, [](lua_State* L) -> int {
        const char* msg = lua_tostring(L, 1);
        printf("[WARN] %s\n", msg ? msg : ""); return 0;
    });
    lua_setglobal(L, "warn");

    // typeof()
    lua_pushcfunction(L, [](lua_State* L) -> int {
        int t = lua_type(L, 1);
        switch (t) {
            case LUA_TNIL: lua_pushstring(L, "nil"); break;
            case LUA_TBOOLEAN: lua_pushstring(L, "boolean"); break;
            case LUA_TNUMBER: lua_pushstring(L, "number"); break;
            case LUA_TSTRING: lua_pushstring(L, "string"); break;
            case LUA_TTABLE: lua_pushstring(L, "table"); break;
            case LUA_TFUNCTION: lua_pushstring(L, "function"); break;
            case LUA_TTHREAD: lua_pushstring(L, "thread"); break;
            case LUA_TUSERDATA: {
                if (lua_getmetatable(L, 1)) {
                    luaL_getmetatable(L, "Instance");
                    if (lua_rawequal(L, -1, -2)) {
                        lua_pop(L, 2);
                        Node* node = *(Node**)(lua_touserdata(L, 1));
                        lua_pushstring(L, node ? node->type : "Instance");
                    } else { lua_pop(L, 2); lua_pushstring(L, "userdata"); }
                } else lua_pushstring(L, "userdata");
                break;
            }
            default: lua_pushstring(L, lua_typename(L, t)); break;
        }
        return 1;
    });
    lua_setglobal(L, "typeof");

    // delay(seconds, func)
    lua_pushcfunction(L, [](lua_State* L) -> int {
        double seconds = lua_tonumber(L, 1);
        if (!lua_isfunction(L, 2)) return 0;
        lua_pushvalue(L, 2);
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        std::thread t([L, ref, seconds]() {
            std::this_thread::sleep_for(std::chrono::milliseconds((int)(seconds*1000)));
            lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
            lua_pcall(L, 0, 0, 0);
            luaL_unref(L, LUA_REGISTRYINDEX, ref);
        });
        t.detach(); return 0;
    });
    lua_setglobal(L, "delay");

    // spawn(func)
    lua_pushcfunction(L, [](lua_State* L) -> int {
        if (!lua_isfunction(L, 1)) return 0;
        lua_pushvalue(L, 1);
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        std::thread t([L, ref]() {
            lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
            lua_pcall(L, 0, 0, 0);
            luaL_unref(L, LUA_REGISTRYINDEX, ref);
        });
        t.detach(); return 0;
    });
    lua_setglobal(L, "spawn");

    // require(module)
    lua_pushcfunction(L, [](lua_State* L) -> int {
        const char* name = lua_tostring(L, 1);
        if (!name) return 0;
        if (dataModel) {
            for (Node* child : dataModel->children) {
                if (strcmp(child->name, name) == 0 && strcmp(child->type, "ModuleScript") == 0) {
                    if (child->source) {
                        if (luaL_dostring(L, child->source) != LUA_OK) lua_pop(L, 1);
                    }
                    return 1;
                }
            }
        }
        return 0;
    });
    lua_setglobal(L, "require");

#endif
}

}  // namespace ScriptEngine