#include "chat_system.h"
#include "font_manager.h"
#include "citro_renderer.h"
#include "render_pipeline.h"
#include <string>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <3ds.h>

namespace ChatSystem {

    // Chat log file handle
    static FILE* g_chatLogFile = nullptr;

    static void openChatLogFile() {
        if (g_chatLogFile) return;
        // Ensure directory exists
        mkdir("sdmc:/OpenBLOX/shareddata", 0755);
        mkdir("sdmc:/OpenBLOX/shareddata/logs", 0755);
        mkdir("sdmc:/OpenBLOX/shareddata/logs/chatlogs", 0755);

        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        char filename[256];
        snprintf(filename, sizeof(filename), "sdmc:/OpenBLOX/shareddata/logs/chatlogs/chatlog-%04d-%02d-%02d_%02d-%02d-%02d.txt",
                 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                 t->tm_hour, t->tm_min, t->tm_sec);
        g_chatLogFile = fopen(filename, "a");
        if (g_chatLogFile) {
            printf("[Chat] Chat log file opened: %s\n", filename);
        }
    }

    void init() {
        consoleInit(GFX_BOTTOM, NULL);
        mode = ChatMode::Safechat;
        inputState = ChatInputState::Idle;
        currentInput.clear();
        messageHistory.clear();
        chatBubbles.clear();
        safechatLoaded = false;
        safechatCurrentNode = &safechatRoot;
        safechatPathDepth = 0;
        g_chatLogFile = nullptr;
    }

    // Recursive XML parser for safechat.xml utterance tree
    static bool parseSafechatFile(const std::string& filepath, SafechatUtterance& root) {
        FILE* f = fopen(filepath.c_str(), "r");
        if (!f) return false;

        fseek(f, 0, SEEK_END);
        long fileSize = ftell(f);
        fseek(f, 0, SEEK_SET);

        char* buffer = new char[fileSize + 1];
        fread(buffer, 1, fileSize, f);
        buffer[fileSize] = '\0';
        fclose(f);

        // Simple state-machine XML parser for safechat format
        struct ParseState {
            char* pos;
            char* end;
            SafechatUtterance* stack[64];
            int stackDepth;
        };

        ParseState state;
        state.pos = buffer;
        state.end = buffer + fileSize;
        state.stackDepth = 0;
        state.stack[0] = &root;

        while (state.pos < state.end) {
            // Skip whitespace
            while (state.pos < state.end && (*state.pos == ' ' || *state.pos == '\n' || *state.pos == '\r' || *state.pos == '\t'))
                state.pos++;
            if (state.pos >= state.end) break;

            // Check for <utterance> tag
            if (strncmp(state.pos, "<utterance>", 10) == 0) {
                state.pos += 10;

                char* nextClose = strstr(state.pos, "</utterance>");
                char* nextOpen = strstr(state.pos, "<utterance>");

                if (nextClose && (!nextOpen || nextOpen > nextClose)) {
                    // Leaf node
                    int textLen = (int)(nextClose - state.pos);
                    while (textLen > 0 && isspace(state.pos[textLen-1])) textLen--;
                    while (state.pos[0] && isspace(state.pos[0])) { state.pos++; textLen--; }
                    if (textLen > 0 && state.stackDepth > 0) {
                        SafechatUtterance* parent = state.stack[state.stackDepth - 1];
                        SafechatUtterance child;
                        child.text.assign(state.pos, textLen);
                        child.isFinal = true;
                        parent->children.push_back(child);
                    }
                    state.pos = nextClose + 11;
                } else {
                    // Parent node
                    int textLen = nextOpen ? (int)(nextOpen - state.pos) : 0;
                    while (textLen > 0 && isspace(state.pos[textLen-1])) textLen--;
                    if (textLen > 0 && state.stackDepth > 0) {
                        SafechatUtterance* parent = state.stack[state.stackDepth - 1];
                        SafechatUtterance child;
                        child.text.assign(state.pos, textLen);
                        child.isFinal = false;
                        parent->children.push_back(child);
                        state.stack[state.stackDepth - 1] = &parent->children.back();
                        state.stackDepth++;
                    }
                    state.pos = nextOpen + 10;
                }
            } else {
                state.pos++;
            }
        }

        delete[] buffer;
        return true;
    }

    bool loadSafechat(const std::string& path) {
        safechatRoot.children.clear();
        safechatRoot.isFinal = false;

        if (parseSafechatFile(path, safechatRoot)) {
            safechatLoaded = true;
            safechatCurrentNode = &safechatRoot;
            printf("[Chat] Safechat loaded from %s (%d root options)\n",
                   path.c_str(), (int)safechatRoot.children.size());
            return true;
        }
        printf("[Chat] Failed to load safechat from %s\n", path.c_str());
        safechatLoaded = false;
        return false;
    }

    void setChatMode(ChatMode m) {
        mode = m;
    }

    ChatMode getChatMode() {
        return mode;
    }

    void openChat() {
        if (mode == ChatMode::Safechat) {
            inputState = ChatInputState::SafechatTree;
            safechatCurrentNode = &safechatRoot;
            safechatSelectedIndex = 0;
            safechatPathDepth = 0;
        } else {
            inputState = ChatInputState::Typing;
            currentInput.clear();
            keyboardRow = 0;
            keyboardCol = 0;
        }
    }

    void closeChat() {
        inputState = ChatInputState::Idle;
        currentInput.clear();
    }

    bool isChatActive() {
        return inputState != ChatInputState::Idle;
    }

    // Convert character to uppercase if shift/caps is active
    static char applyShift(char c) {
        if (keyboardShift || keyboardCaps) {
            if (c >= 'a' && c <= 'z') return c - 'a' + 'A';
        }
        return c;
    }

    ChatMessage processChatInput() {
        ChatMessage result = {"", "", 0, MessageType::Player, 0, 0};

        if (inputState == ChatInputState::Idle) {
            // Check for Y button to open chat
            if (hidKeysDown() & KEY_Y) {
                openChat();
            }
            return result;
        }

        // B button always closes chat
        if (hidKeysDown() & KEY_B) {
            closeChat();
            return result;
        }

        if (inputState == ChatInputState::SafechatTree) {
            // Safechat tree navigation
            int maxItems = std::min(8, (int)safechatCurrentNode->children.size());
            int totalCount = (int)safechatCurrentNode->children.size();

            if (hidKeysDown() & KEY_UP) {
                safechatSelectedIndex--;
                if (safechatSelectedIndex < 0) safechatSelectedIndex = maxItems - 1;
                if (safechatSelectedIndex < safechatScrollOffset) safechatScrollOffset = safechatSelectedIndex;
            }
            if (hidKeysDown() & KEY_DOWN) {
                safechatSelectedIndex++;
                if (safechatSelectedIndex >= totalCount) safechatSelectedIndex = 0;
                if (safechatSelectedIndex >= safechatScrollOffset + maxItems) safechatScrollOffset = safechatSelectedIndex - maxItems + 1;
            }

            if (hidKeysDown() & KEY_A) {
                int idx = safechatScrollOffset + safechatSelectedIndex;
                if (idx >= 0 && idx < totalCount) {
                    SafechatUtterance* selected = &safechatCurrentNode->children[idx];
                    if (selected->isFinal || selected->children.empty()) {
                        // Final utterance - send it
                        result.playerName = "Player";
                        result.message = selected->text;
                        result.timestamp = (u32)time(NULL);
                        addChatMessage(result.playerName, result.message);
                        closeChat();
                    } else {
                        // Has children - go deeper
                        safechatPathStack[safechatPathDepth].parent = safechatCurrentNode;
                        safechatPathStack[safechatPathDepth].childIndex = idx;
                        safechatPathDepth++;
                        safechatCurrentNode = selected;
                        safechatSelectedIndex = 0;
                    }
                }
            }

            // D-Right goes back up the tree
            if (hidKeysDown() & KEY_RIGHT && safechatPathDepth > 0) {
                safechatPathDepth--;
                safechatCurrentNode = safechatPathStack[safechatPathDepth].parent;
                safechatSelectedIndex = safechatPathStack[safechatPathDepth].childIndex;
            }

        } else if (inputState == ChatInputState::Typing) {
            // Free-text keyboard input
            if (hidKeysDown() & KEY_UP) {
                keyboardRow--;
                if (keyboardRow < 0) keyboardRow = 3;
            }
            if (hidKeysDown() & KEY_DOWN) {
                keyboardRow++;
                if (keyboardRow >= 4) keyboardRow = 0;
            }
            if (hidKeysDown() & KEY_LEFT) {
                keyboardCol--;
                if (keyboardCol < 0) keyboardCol = (int)strlen(keyboardRows[keyboardRow]) - 1;
            }
            if (hidKeysDown() & KEY_RIGHT) {
                keyboardCol++;
                if (keyboardCol >= (int)strlen(keyboardRows[keyboardRow])) keyboardCol = 0;
            }

            // L/R for shift/caps
            if (hidKeysDown() & KEY_L) {
                keyboardShift = !keyboardShift;
                keyboardCaps = false;
            }
            if (hidKeysDown() & KEY_R) {
                keyboardCaps = !keyboardCaps;
                keyboardShift = false;
            }

            // A to type character
            if (hidKeysDown() & KEY_A) {
                char c = applyShift(keyboardRows[keyboardRow][keyboardCol]);
                if (c == '-') c = ' ';  // Space on dash key
                currentInput += c;
                keyboardShift = false;
            }

            // X to delete
            if (hidKeysDown() & KEY_X) {
                if (!currentInput.empty()) {
                    currentInput.pop_back();
                }
            }

            // Y to send message
            if (hidKeysDown() & KEY_Y && !currentInput.empty()) {
                // Check for chat command first
                if (!processChatCommand("Player", currentInput)) {
                    result.playerName = "Player";
                    result.message = currentInput;
                    result.timestamp = (u32)time(NULL);
                    addChatMessage(result.playerName, result.message, MessageType::Player);
                }
                closeChat();
            }
        }

        return result;
    }

    void drawChatOverlay() {
        if (inputState == ChatInputState::Idle) return;

        // Draw chat overlay background
        // Clear bottom screen for chat
        consoleClear();

        if (inputState == ChatInputState::SafechatTree) {
            // Draw safechat tree
            consoleSelect(NULL);
            iprintf("n--- Safechat ---nn");

            int maxItems = std::min(8, (int)safechatCurrentNode->children.size());
            for (int i = 0; i < maxItems; i++) {
                int idx = safechatScrollOffset + i;
                if (idx >= (int)safechatCurrentNode->children.size()) break;
                SafechatUtterance* item = &safechatCurrentNode->children[idx];
                if (i == safechatSelectedIndex) iprintf("-> "); else iprintf("   ");
                iprintf("%s%sn", item->text.c_str(), item->children.empty() ? "" : " ...");
            }

            if (safechatPathDepth > 0) {
                iprintf("n[RIGHT] Back");
            }
            iprintf(" [A] Select");
            if (safechatCurrentNode->children.size() > 8) {
                iprintf(" [%d/%d]", safechatScrollOffset + safechatSelectedIndex + 1, (int)safechatCurrentNode->children.size());
            }
            iprintf(" [B] Cancel");

        } else if (inputState == ChatInputState::Typing) {
            // Draw free-text keyboard
            consoleSelect(NULL);
            iprintf("nCurrent: %sn", currentInput.c_str());
            iprintf("[Y] Send [B] Cancel [X] Delnn");

            for (int row = 0; row < 4; row++) {
                for (int col = 0; col < (int)strlen(keyboardRows[row]); col++) {
                    if (row == keyboardRow && col == keyboardCol) {
                        printf("[");
                    } else {
                        printf(" ");
                    }
                    char c = keyboardRows[row][col];
                    if (keyboardShift || keyboardCaps) {
                        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
                    }
                    printf("%c", c);
                    if (row == keyboardRow && col == keyboardCol) {
                        printf("]");
                    } else {
                        printf(" ");
                    }
                }
                printf("n");
            }
            printf("n[L] Shift [R] Caps");
        }
    }

    void drawChatHistory(const char* fontName /*= nullptr*/) {
        // Draw recent chat messages on top screen (fade out after 10 seconds)
        int startIdx = std::max(0, (int)messageHistory.size() - 4);
        for (int i = startIdx; i < (int)messageHistory.size(); i++) {
            const ChatMessage& msg = messageHistory[i];
            u32 age = (u32)time(NULL) - msg.timestamp;
            if (age > 10) continue; // Hide messages older than 10 seconds

            int yOff = 20 + (i - startIdx) * 20;
            // Color based on message type
            int nameR = 100, nameG = 200, nameB = 255; // Default: blue
            if (msg.type == MessageType::System) { nameR = 100; nameG = 255; nameB = 100; } // Green
            else if (msg.type == MessageType::Me) { nameR = 200; nameG = 200; nameB = 200; } // Gray
            else if (msg.type == MessageType::Emote) { nameR = 255; nameG = 200; nameB = 100; } // Orange

            FontManager::drawTextTop(
                msg.playerName.c_str(), 10, yOff, nameR, nameG, nameB, FontSize::Size12);
            int nameWidth = FontManager::getTextWidth(msg.playerName, FontSize::Size12);
            FontManager::drawTextTop(
                msg.message.c_str(), 10 + nameWidth + 5, yOff, 255, 255, 255, FontSize::Size12);
        }
    }

    void addChatMessage(const std::string& playerName, const std::string& message) {
        addChatMessage(playerName, message, MessageType::Player);
    }

    void addChatMessage(const std::string& playerName, const std::string& message, MessageType type) {
        ChatMessage msg;
        msg.playerName = playerName;
        msg.message = message;
        msg.timestamp = (u32)time(NULL);
        msg.type = type;
        msg.positionX = 0;
        msg.positionY = 0;
        messageHistory.push_back(msg);

        // Keep only last 50 messages
        if (messageHistory.size() > 50) {
            messageHistory.erase(messageHistory.begin());
        }

        // Log to file: sdmc:/OpenBLOX/shareddata/logs/chatlogs/chatlog-YYYY-MM-DD_HH-mm-ss.txt
        time_t now = time(NULL);
        struct tm* timeinfo = localtime(&now);
        char logPath[256];
        snprintf(logPath, sizeof(logPath), "sdmc:/OpenBLOX/shareddata/logs/chatlogs/chatlog-%04d-%02d-%02d_%02d-%02d-%02d.txt",
                 timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                 timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        
        // Create directory if it doesn't exist (simple mkdir call)
        mkdir("sdmc:/OpenBLOX/shareddata", 0755);
        mkdir("sdmc:/OpenBLOX/shareddata/logs", 0755);
        mkdir("sdmc:/OpenBLOX/shareddata/logs/chatlogs", 0755);

        FILE* f = fopen(logPath, "a");
        if (f) {
            const char* typeStr = "Player";
            if (type == MessageType::System) typeStr = "System";
            else if (type == MessageType::Me) typeStr = "Me";
            else if (type == MessageType::Emote) typeStr = "Emote";
            
            fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] %s: %s\n",
                    timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
                    typeStr, playerName.c_str(), message.c_str());
            fclose(f);
        }

        // Print to console based on type
        const char* typePrefix = "";
        switch (type) {
            case MessageType::System: typePrefix = "[SYSTEM] "; break;
            case MessageType::Me: typePrefix = "* "; break;
            case MessageType::Emote: typePrefix = "[EMOTE] "; break;
            default: break;
        }
        printf("[Chat] %s%s: %s\n", typePrefix, playerName.c_str(), message.c_str());

        // Add bubble for visible messages
        if (type != MessageType::System) {
            addChatBubble(playerName, message, 100, 200);
        }
    }

    const SafechatUtterance* getSafechatRoot() {
        if (!safechatLoaded) return nullptr;
        return &safechatRoot;
    }

    std::string getCurrentInputText() {
        return currentInput;
    }

    bool isWithinProximity(float distance, float maxDistance) {
        return distance <= maxDistance;
    }

    bool processChatCommand(const std::string& playerName, const std::string& message) {
        if (message.empty() || message[0] != '/') return false;

        // /me - action message
        if (message.substr(0, 3) == "/me") {
            std::string action = message.substr(3);
            if (!action.empty() && action[0] == ' ') action = action.substr(1);
            if (!action.empty()) {
                ChatMessage msg;
                msg.playerName = playerName;
                msg.message = "* " + playerName + " " + action;
                msg.timestamp = (u32)time(NULL);
                msg.type = MessageType::Me;
                messageHistory.push_back(msg);
                printf("[Chat] * %s %s\n", playerName.c_str(), action.c_str());
            }
            return true;
        }

        // /emote - trigger emote
        if (message.substr(0, 7) == "/emote") {
            std::string emoteName = message.substr(7);
            if (!emoteName.empty() && emoteName[0] == ' ') emoteName = emoteName.substr(1);
            addChatMessage(playerName, "[Emote: " + emoteName + "]", MessageType::Emote);
            return true;
        }

        // /sit - sit down
        if (message == "/sit") {
            addChatMessage("System", playerName + " is now sitting", MessageType::System);
            return true;
        }

        // /fly - toggle flying
        if (message == "/fly") {
            addChatMessage("System", playerName + " toggled fly mode", MessageType::System);
            return true;
        }

        return false;
    }

    void addChatBubble(const std::string& playerName, const std::string& message, int x, int y) {
        ChatBubble bubble;
        bubble.playerName = playerName;
        bubble.message = message;
        bubble.positionX = x;
        bubble.positionY = y;
        bubble.spawnTime = (u32)time(NULL);
        bubble.type = MessageType::Player;
        chatBubbles.push_back(bubble);

        // Keep max 10 bubbles
        if (chatBubbles.size() > 10) {
            chatBubbles.erase(chatBubbles.begin());
        }
    }

    void drawChatBubbles() {
        u32 currentTime = (u32)time(NULL);
        for (auto it = chatBubbles.begin(); it != chatBubbles.end();) {
            u32 age = currentTime - it->spawnTime;
            if (age > 6) {
                it = chatBubbles.erase(it);
                continue;
            }

            // Calculate opacity (fade out last 2 seconds)
            float alpha = 1.0f;
            if (age > 4) alpha = (6.0f - age) / 2.0f;

            // Draw bubble background
            int bubbleWidth = FontManager::getTextWidth(it->message, FontSize::Size12) + 16;
            int bubbleHeight = 24;

            float r, g, b;
            switch (it->type) {
                case MessageType::Player: r = 200; g = 220; b = 255; break; // Light blue
                case MessageType::System: r = 100; g = 200; b = 100; break; // Green
                case MessageType::Me:     r = 200; g = 200; b = 200; break; // Gray
                case MessageType::Emote:  r = 255; g = 200; b = 100; break; // Orange
            }

            // Draw player name above bubble
            FontManager::drawTextTop(
                it->playerName.c_str(),
                it->positionX + 8, it->positionY,
                255, 255, 255, FontSize::Size12
            );

            // Draw bubble background
            RenderPipeline::drawRect(
                (float)it->positionX, (float)it->positionY + 8,
                (float)bubbleWidth, (float)bubbleHeight,
                (r * alpha) / 255.0f, (g * alpha) / 255.0f, (b * alpha) / 255.0f,
                1.0f
            );

            // Draw message text
            FontManager::drawTextTop(
                it->message.c_str(),
                it->positionX + 8, it->positionY + 16,
                0, 0, 0, FontSize::Size12
            );

            it++;
        }
    }

    void shutdown() {
        if (g_chatLogFile) {
            fclose(g_chatLogFile);
            g_chatLogFile = nullptr;
        }
        messageHistory.clear();
        chatBubbles.clear();
        init();
    }

    void logChatToFile(const std::string& playerName, const std::string& message, u32 timestamp) {
        openChatLogFile();
        if (g_chatLogFile) {
            time_t now = (time_t)timestamp;
            struct tm* t = localtime(&now);
            fprintf(g_chatLogFile, "[%04d-%02d-%02d %02d:%02d:%02d] %s: %s\n",
                    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                    t->tm_hour, t->tm_min, t->tm_sec,
                    playerName.c_str(), message.c_str());
            fflush(g_chatLogFile);
        }
    }

}