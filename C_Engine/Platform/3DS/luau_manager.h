/*
** Luau Hybrid VM Manager
** Routes scripts between Lua 5.1 and Luau VMs
*/

#ifndef luau_manager_h
#define luau_manager_h

#include <string>
#include <vector>
#include <map>

enum class LuauVMType {
    Lua51,    // Standard Lua 5.1
    Luau      // Roblox Luau (type-checked)
};

struct LuauScriptInfo {
    std::string path;
    LuauVMType vmType;
    bool typeChecked;
    bool hasStrictMode;
};

namespace LuauManager {

// Initialize the hybrid VM system
bool init();

// Shutdown and cleanup
void shutdown();

// Detect whether a script should use Luau or Lua 5.1
LuauVMType detectVMType(const std::string& filePath, const std::string& source);

// Preprocess Luau-specific features into Lua 5.1 compatible code
std::string preprocessLuau(const std::string& source);

// Type annotation stripping (converts `local x: number` to `local x`)
std::string stripTypeAnnotations(const std::string& source);

// Generic type handling (converts `Table<T>` to `Table`)
std::string convertGenerics(const std::string& source);

// Continue statement handling (converts `continue` to `break`)
std::string convertContinueStatements(const std::string& source);

// Compound assignment handling (converts `x += 1` to `x = x + 1`)
std::string convertCompoundAssignments(const std::string& source);

// Numeric literal handling (removes underscores: `1_000` -> `1000`)
std::string normalizeNumericLiterals(const std::string& source);

// Check if source uses Luau-specific features
bool usesLuauFeatures(const std::string& source);

// Get script info with VM type detection
LuauScriptInfo getScriptInfo(const std::string& filePath, const std::string& source);

// Register a custom Luau module
void registerModule(const std::string& name, const std::string& source);

// Execute script in appropriate VM
int executeScript(const std::string& name, const std::string& source);

// Get statistics
void getStats(int& lua51Count, int& luauCount);

} // namespace LuauManager

#endif // luau_manager_h
