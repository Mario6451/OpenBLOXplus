/*
** Luau Hybrid VM Manager Implementation
** Routes scripts between Lua 5.1 and Luau VMs
*/

#include "luau_manager.h"
#include "script_engine.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>

// External Lua state reference
extern "C" {
    #include "../../ThirdParty/lua51/lua.h"
    #include "../../ThirdParty/lua51/lauxlib.h"
    #include "../../ThirdParty/lua51/lualib.h"
}

namespace LuauManager {

// Internal state
static bool initialized = false;
static int lua51ScriptCount = 0;
static int luauScriptCount = 0;
static std::map<std::string, std::string> customModules;

bool init() {
    if (initialized) return true;
    initialized = true;
    lua51ScriptCount = 0;
    luauScriptCount = 0;
    customModules.clear();
    printf("[LuauManager] Hybrid VM system initialized\n");
    return true;
}

void shutdown() {
    if (!initialized) return;
    lua51ScriptCount = 0;
    luauScriptCount = 0;
    customModules.clear();
    initialized = false;
    printf("[LuauManager] Hybrid VM system shut down\n");
}

LuauVMType detectVMType(const std::string& filePath, const std::string& source) {
    // Check file extension
    if (filePath.size() >= 5 && filePath.substr(filePath.size() - 5) == ".luau") {
        return LuauVMType::Luau;
    }
    
    // Check for --!strict or --!nostart annotations
    if (source.find("--!strict") != std::string::npos ||
        source.find("--!nostart") != std::string::npos ||
        source.find("--!nolibrary") != std::string::npos) {
        return LuauVMType::Luau;
    }
    
    // Check for Luau-specific features
    if (usesLuauFeatures(source)) {
        return LuauVMType::Luau;
    }
    
    // Default to Lua 5.1
    return LuauVMType::Lua51;
}

bool usesLuauFeatures(const std::string& source) {
    // Type annotations
    if (source.find(": string") != std::string::npos ||
        source.find(": number") != std::string::npos ||
        source.find(": boolean") != std::string::npos ||
        source.find(": table") != std::string::npos ||
        source.find(": function") != std::string::npos) {
        return true;
    }
    
    // Generics
    if (source.find("<") != std::string::npos &&
        source.find(">") != std::string::npos) {
        // Simple heuristic for generic detection
        size_t lt = source.find("<");
        if (lt != std::string::npos) {
            size_t gt = source.find(">", lt);
            if (gt != std::string::npos) {
                std::string generic = source.substr(lt + 1, gt - lt - 1);
                if (generic.find(":") != std::string::npos ||
                    generic.find(",") != std::string::npos) {
                    return true;
                }
            }
        }
    }
    
    // Compound assignments
    if (source.find("+=") != std::string::npos ||
        source.find("-=") != std::string::npos ||
        source.find("*=") != std::string::npos ||
        source.find("/=") != std::string::npos ||
        source.find("//=") != std::string::npos) {
        return true;
    }
    
    // Numeric literals with underscores
    if (source.find("1_") != std::string::npos ||
        source.find("100_") != std::string::npos) {
        return true;
    }
    
    // Continue statements
    if (source.find("continue") != std::string::npos) {
        return true;
    }
    
    return false;
}

std::string stripTypeAnnotations(const std::string& source) {
    std::string result;
    std::string line;
    
    for (size_t i = 0; i < source.size(); i++) {
        if (source[i] == '\n') {
            // Process line
            std::string convertedLine;
            bool inTypeAnnotation = false;
            bool afterLocal = false;
            bool afterFunction = false;
            
            for (size_t j = 0; j < line.size(); j++) {
                // Detect type annotations
                if (line[j] == ':' && j > 0) {
                    // Check if this is a type annotation
                    std::string before = line.substr(0, j);
                    // Trim trailing spaces
                    while (!before.empty() && (before.back() == ' ' || before.back() == '\t')) {
                        before.pop_back();
                    }
                    
                    // If preceded by identifier, this is likely a type annotation
                    if (!before.empty() && 
                        (before.back() >= 'a' && before.back() <= 'z' ||
                         before.back() >= 'A' && before.back() <= 'Z' ||
                         before.back() >= '0' && before.back() <= '9' ||
                         before.back() == '_' || before.back() == ']')) {
                        inTypeAnnotation = true;
                        continue;
                    }
                }
                
                // Exit type annotation mode
                if (inTypeAnnotation) {
                    if (line[j] == '=' || line[j] == ',' || line[j] == '\n') {
                        inTypeAnnotation = false;
                        if (line[j] == '=') convertedLine += '=';
                    }
                    continue;
                }
                
                convertedLine += line[j];
            }
            
            result += convertedLine;
            result += '\n';
            line.clear();
        } else {
            line += source[i];
        }
    }
    
    result += line;
    return result;
}

std::string convertGenerics(const std::string& source) {
    std::string result = source;
    
    // Remove generic parameters
    size_t pos = 0;
    while ((pos = result.find("<", pos)) != std::string::npos) {
        // Check if this is a generic type
        std::string before = result.substr(0, pos);
        if (!before.empty() && (before.back() >= 'A' && before.back() <= 'Z')) {
            size_t end = result.find(">", pos);
            if (end != std::string::npos) {
                result.erase(pos, end - pos + 1);
            }
        }
    }
    
    return result;
}

bool isIdentChar(char c) {
    return ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '_');
}

bool isDigit(char c) {
    return (c >= '0' && c <= '9');
}

std::string convertContinueStatements(const std::string& source) {
    std::string result;
    result.reserve(source.size() + 100);
    
    for (size_t i = 0; i < source.size(); i++) {
        // Skip string literals
        if (source[i] == '"' || source[i] == '\'') {
            char quote = source[i];
            result += source[i];
            while (i + 1 < source.size() && source[i+1] != quote && source[i+1] != '\n') {
                i++;
                result += source[i];
                if (source[i] == '\\') {
                    i++;
                    if (i < source.size()) result += source[i];
                }
            }
            continue;
        }
        
        // Skip single-line comments
        if (i + 1 < source.size() && source[i] == '-' && source[i+1] == '-') {
            result += source[i];
            i++;
            result += source[i];
            i++;
            while (i < source.size() && source[i] != '\n') {
                result += source[i];
                i++;
            }
            continue;
        }
        
        // Check for whole-word "continue" keyword
        if (source[i] == 'c' && source.substr(i, 8) == "continue") {
            bool beforeOk = (i == 0) || !isIdentChar(source[i-1]);
            size_t afterPos = i + 8;
            bool afterOk = (afterPos >= source.size()) || !isIdentChar(source[afterPos]);
            if (beforeOk && afterOk) {
                result += "break";
                i += 7;
                continue;
            }
        }
        result += source[i];
    }
    return result;
}

std::string convertCompoundAssignments(const std::string& source) {
    std::string result;
    std::string line;
    
    for (size_t i = 0; i < source.size(); i++) {
        if (source[i] == '\n') {
            // Process compound assignments
            std::string convertedLine;
            for (size_t j = 0; j < line.size(); j++) {
                if (j + 1 < line.size()) {
                    if (line[j] == '+' && line[j+1] == '=') {
                        // Extract variable name
                        size_t k = j - 1;
                        while (k > 0 && (line[k-1] == ' ' || line[k-1] == '\t')) k--;
                        std::string var;
                        while (k > 0 && ((line[k-1] >= 'a' && line[k-1] <= 'z') ||
                                       (line[k-1] >= 'A' && line[k-1] <= 'Z') ||
                                       (line[k-1] >= '0' && line[k-1] <= '9') ||
                                       line[k-1] == '_')) k--;
                        var = line.substr(k, j - k);
                        convertedLine += var + " = " + var + " + ";
                        j++; continue;
                    }
                    if (line[j] == '-' && line[j+1] == '=') {
                        size_t k = j - 1;
                        while (k > 0 && (line[k-1] == ' ' || line[k-1] == '\t')) k--;
                        std::string var;
                        while (k > 0 && ((line[k-1] >= 'a' && line[k-1] <= 'z') ||
                                       (line[k-1] >= 'A' && line[k-1] <= 'Z') ||
                                       (line[k-1] >= '0' && line[k-1] <= '9') ||
                                       line[k-1] == '_')) k--;
                        var = line.substr(k, j - k);
                        convertedLine += var + " = " + var + " - ";
                        j++; continue;
                    }
                    if (line[j] == '*' && line[j+1] == '=') {
                        size_t k = j - 1;
                        while (k > 0 && (line[k-1] == ' ' || line[k-1] == '\t')) k--;
                        std::string var;
                        while (k > 0 && ((line[k-1] >= 'a' && line[k-1] <= 'z') ||
                                       (line[k-1] >= 'A' && line[k-1] <= 'Z') ||
                                       (line[k-1] >= '0' && line[k-1] <= '9') ||
                                       line[k-1] == '_')) k--;
                        var = line.substr(k, j - k);
                        convertedLine += var + " = " + var + " * ";
                        j++; continue;
                    }
                    if (line[j] == '/' && line[j+1] == '=') {
                        size_t k = j - 1;
                        while (k > 0 && (line[k-1] == ' ' || line[k-1] == '\t')) k--;
                        std::string var;
                        while (k > 0 && ((line[k-1] >= 'a' && line[k-1] <= 'z') ||
                                       (line[k-1] >= 'A' && line[k-1] <= 'Z') ||
                                       (line[k-1] >= '0' && line[k-1] <= '9') ||
                                       line[k-1] == '_')) k--;
                        var = line.substr(k, j - k);
                        convertedLine += var + " = " + var + " / ";
                        j++; continue;
                    }
                }
                convertedLine += line[j];
            }
            result += convertedLine;
            result += '\n';
            line.clear();
        } else {
            line += source[i];
        }
    }
    result += line;
    return result;
}

 std::string normalizeNumericLiterals(const std::string& source) {
    std::string result;
    result.reserve(source.size());
    
    for (size_t i = 0; i < source.size(); i++) {
        // Skip string literals
        if (source[i] == '"' || source[i] == '\'') {
            char quote = source[i];
            result += source[i];
            while (i + 1 < source.size() && source[i+1] != quote && source[i+1] != '\n') {
                i++;
                result += source[i];
                if (source[i] == '\\') {
                    i++;
                    if (i < source.size()) result += source[i];
                }
            }
            continue;
        }
        
        // Skip single-line comments
        if (i + 1 < source.size() && source[i] == '-' && source[i+1] == '-') {
            result += source[i]; i++;
            result += source[i]; i++;
            while (i < source.size() && source[i] != '\n') {
                result += source[i];
                i++;
            }
            continue;
        }
        
        // Skip multi-line comments
        if (i + 1 < source.size() && source[i] == '-' && source[i+1] == '-' && i + 2 < source.size() && source[i+2] == '[') {
            size_t bracketCount = 0;
            size_t j = i + 2;
            while (j < source.size() && source[j] == '=') bracketCount++;
            j += (source[j] == '[') ? 1 : 0;
            while (j < source.size()) {
                if (source[j] == ']' && j + 1 + bracketCount <= source.size()) {
                    bool match = true;
                    for (size_t k = 0; k <= bracketCount; k++) {
                        if (k == bracketCount) {
                            if (source[j + k] != ']') { match = false; break; }
                        } else {
                            if (source[j + k] != '=') { match = false; break; }
                        }
                    }
                    if (match) { j += bracketCount + 2; break; }
                }
                j++;
            }
            result += source.substr(i, j - i);
            i = j;
            continue;
        }
        
        // Process numeric literal (starts with digit)
        if (isDigit(source[i])) {
            // Copy entire numeric literal, stripping underscores
            while (i < source.size() && (isDigit(source[i]) || source[i] == '.' || source[i] == '_' || source[i] == 'e' || source[i] == 'E')) {
                if (source[i] != '_') result += source[i];
                i++;
                // Handle exponent sign
                if (i < source.size() && (source[i] == '+' || source[i] == '-') && 
                    (source[i-1] == 'e' || source[i-1] == 'E')) {
                    result += source[i];
                    i++;
                } else if (!(isDigit(source[i]) || source[i] == '.' || source[i] == '_' || source[i] == 'e' || source[i] == 'E')) {
                    break;
                }
            }
            continue;
        }
        result += source[i];
    }
    return result;
}

std::string preprocessLuau(const std::string& source) {
    std::string processed = source;
    
    // Apply all Luau -> Lua 5.1 conversions
    processed = stripTypeAnnotations(processed);
    processed = convertGenerics(processed);
    processed = convertContinueStatements(processed);
    processed = convertCompoundAssignments(processed);
    processed = normalizeNumericLiterals(processed);
    
    return processed;
}

LuauScriptInfo getScriptInfo(const std::string& filePath, const std::string& source) {
    LuauScriptInfo info;
    info.path = filePath;
    info.vmType = detectVMType(filePath, source);
    info.typeChecked = (info.vmType == LuauVMType::Luau);
    info.hasStrictMode = (source.find("--!strict") != std::string::npos);
    return info;
}

void registerModule(const std::string& name, const std::string& source) {
    customModules[name] = source;
    printf("[LuauManager] Registered module: %s\n", name.c_str());
}

int executeScript(const std::string& name, const std::string& source) {
    LuauVMType vmType = detectVMType(name, source);
    
    if (vmType == LuauVMType::Luau) {
        luauScriptCount++;
        printf("[LuauManager] Executing %s with Luau preprocessing\n", name.c_str());
        
        // Preprocess Luau code to Lua 5.1 compatible format
        std::string processed = preprocessLuau(source);
        
        // Execute via Lua 5.1 VM with preprocessed code
        return ScriptEngine::loadScriptString(name, processed) ? 0 : -1;
    } else {
        lua51ScriptCount++;
        printf("[LuauManager] Executing %s with Lua 5.1\n", name.c_str());
        
        // Execute directly with Lua 5.1
        return ScriptEngine::loadScriptString(name, source) ? 0 : -1;
    }
}

void getStats(int& lua51Count, int& luauCount) {
    lua51Count = lua51ScriptCount;
    luauCount = luauScriptCount;
}

} // namespace LuauManager
