#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <3ds.h>

// Chat system with safechat support for 3DS OpenBLOX
// Safechat uses a tree of utterances from safechat.xml

namespace ChatSystem {

    // Chat message type
    enum class MessageType {
        Player,    // Regular player chat (blue bubbles)
        System,    // System messages (green bubbles)
        Me,        // /me action messages (italic, white)
        Emote      // /emote messages (colored)
    };

    // A single chat message
    struct ChatMessage {
        std::string playerName;
        std::string message;
        u32 timestamp;
        MessageType type;
        int positionX;
        int positionY;
    };

    // Chat bubble (renders above character head)
    struct ChatBubble {
        std::string playerName;
        std::string message;
        int positionX;
        int positionY;
        u32 spawnTime;
        MessageType type;
    };

    // Safechat utterance node (from safechat.xml)
    struct SafechatUtterance {
        std::string text;                          // Display text
        std::vector<SafechatUtterance> children;   // Sub-options
        bool isFinal;                              // No children = selectable
    };

    // Safechat path stack entry
    struct SafechatPathEntry {
        SafechatUtterance* parent;
        int childIndex;
    };

    // Chat mode
    enum class ChatMode {
        Free,           // Free-text input (physical keyboard or on-screen)
        Safechat        // Utterance tree navigation
    };

    // Chat input state
    enum class ChatInputState {
        Idle,           // Chat not active
        Typing,         // Free-text input active
        SafechatTree    // Navigating safechat utterance tree
    };

    // Static member variables
    static ChatMode mode;
    static ChatInputState inputState;
    static std::string currentInput;
    static std::vector<ChatMessage> messageHistory;
    static std::vector<ChatBubble> chatBubbles;
    static bool safechatLoaded;
    static SafechatUtterance* safechatCurrentNode;
    static SafechatUtterance safechatRoot;
    static int safechatPathDepth;
    static int safechatSelectedIndex;
    static int safechatScrollOffset;
    static SafechatPathEntry safechatPathStack[64];
    static int keyboardRow;
    static int keyboardCol;
    static bool keyboardShift;
    static bool keyboardCaps;
    static const char* keyboardRows[] = {
        "qwertyuiop",
        "asdfghjkl",
        "zxcvbnm-",
        "1234567890"
    };

    // Initialize the chat system
    void init();

    // Load safechat utterance tree from safechat.xml
    bool loadSafechat(const std::string& path);

    // Set chat mode (free or safechat)
    void setChatMode(ChatMode mode);

    // Get current chat mode
    ChatMode getChatMode();

    // Open chat input (returns true to toggle visibility)
    void openChat();

    // Close chat input
    void closeChat();

    // Check if chat input is currently active
    bool isChatActive();

    // Process chat input (call once per frame when chat is active)
    // Returns ChatMessage if a message was sent, empty string otherwise
    ChatMessage processChatInput();

    // Draw chat overlay on bottom screen
    void drawChatOverlay();

    // Draw chat message history on top screen (fade out after 5 seconds)
    void drawChatHistory(const char* fontName = nullptr);

    // Add a chat message to history
    void addChatMessage(const std::string& playerName, const std::string& message);
    void addChatMessage(const std::string& playerName, const std::string& message, MessageType type);

    // Get safechat root utterance for tree navigation
    const SafechatUtterance* getSafechatRoot();

    // Get current input text (for free chat)
    std::string getCurrentInputText();

    // Proximity-based chat (returns true if message is within range)
    bool isWithinProximity(float distance, float maxDistance = 30.0f);

    // Process chat commands (/me, /emote, /sit, /fly)
    bool processChatCommand(const std::string& playerName, const std::string& message);

    // Chat bubbles
    void addChatBubble(const std::string& playerName, const std::string& message, int x, int y);
    void drawChatBubbles();

    // Shutdown
    void shutdown();

    // Chat file logging
    void logChatToFile(const std::string& playerName, const std::string& message, u32 timestamp);
}