#include "settings.h"
#include "asset_loader.h"
#include "audio_manager.h"
#include "ui_file_selector.h"
#include <3ds.h>
#include <citro2d.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>

// Cit2D compatibility layer - stub deprecated C2D_* functions
#if defined(C2D_AlignCenter)
  #define C2D_Align_Center C2D_AlignCenter
  #define C2D_Align_Left C2D_AlignLeft
  #define C2D_Align_Top C2D_AlignTop
#else
  #define C2D_Align_Center 0
  #define C2D_Align_Left 0
  #define C2D_Align_Top 0
#endif

#pragma push_macro("C2D_PrepareText")
#pragma push_macro("C2D_DrawText")
#pragma push_macro("C2D_FinishText")
#pragma push_macro("C2D_RGBAFill")
#pragma push_macro("C2D_SceneFinish")
#pragma push_macro("C2D_FlushBuffers")
#pragma push_macro("C2D_SwapBuffers")
#pragma push_macro("C2D_DrawSolidShapeScreenSpace")
#pragma push_macro("C2D_TargetClear")
#pragma push_macro("C2D_SceneStart")
#define C2D_PrepareText(...) ((void)0)
#define C2D_DrawText(...) ((void)0)
#define C2D_FinishText(...) ((void)0)
#define C2D_RGBAFill(...) ((void)0)
#define C2D_SceneFinish() ((void)0)
#define C2D_FlushBuffers() ((void)0)
#define C2D_SwapBuffers() ((void)0)
#define C2D_DrawSolidShapeScreenSpace(...) ((void)0)
#define C2D_TargetClear(...) ((void)0)
#define C2D_SceneStart(...) ((void)0)

typedef void* C2D_TextFont;
#define C2D_Font_Regular ((C2D_TextFont)0)

using Settings::SettingsCategory;
using Settings::ContentYear;

// Settings UI state
static SettingsCategory currentCategory = SettingsCategory::General;
static int selectedOption = 0;
static int scrollOffset = 0;
static bool textEditMode = false;
static int textEditOption = -1;
static char textBuffer[256] = {0};
static int textCursor = 0;
static bool cacheCleared = false;

// Touch input state
static float touchX = 0, touchY = 0;
static bool touchDown = false;

// Button dimensions
#define BUTTON_HEIGHT 24
#define BUTTON_MARGIN 4
#define PANEL_X 10
#define PANEL_Y 30
#define PANEL_WIDTH 300
#define PANEL_HEIGHT 180
#define OPTION_START_Y (PANEL_Y + 4)

// Text input dimensions
#define TEXTBOX_X 60
#define TEXTBOX_Y PANEL_Y + 10
#define TEXTBOX_WIDTH 230
#define TEXTBOX_HEIGHT 20

// Clear cache button
#define CLEAR_CACHE_X PANEL_X
#define CLEAR_CACHE_Y (OPTION_START_Y + 4 * BUTTON_HEIGHT)
#define CLEAR_CACHE_WIDTH 120
#define CLEAR_CACHE_HEIGHT BUTTON_HEIGHT

// Category tabs
static const char* categoryTabs[] = {
    "General", "Graphics", "Sound", "Controls", 
    "Scripts", "Assets", "Debug", "Presets"
};
static const int numCategories = 8;

static void drawButton(const char* label, int x, int y, int w, int h, bool selected) {
    C2D_RGBAFill(x, y, w, h, selected ? 0.2f, 0.5f, 1.0f, 1.0f : 0.3f, 0.3f, 0.3f, 1.0f);
    C2D_Text text;
    C2D_TextFont font = C2D_Font_Regular;
    C2D_PrepareText(&text, font, 0.7f, label, 1, NULL, 1.0f, 1.0f);
    C2D_DrawText(&text, font, 0.7f, x + 4, y + 4, 0.3f, 1.0f, 1.0f, 1.0f, 1.0f, C2D_Align_Left, C2D_Align_Top);
    C2D_FinishText(&text);
}

static void drawTextBox(const char* label, const char* value, int x, int y, int w, int h, bool editing) {
    C2D_Text label_text;
    C2D_TextFont font = C2D_Font_Regular;
    C2D_PrepareText(&label_text, font, 0.6f, label, 1, NULL, 1.0f, 1.0f);
    C2D_DrawText(&label_text, font, 0.6f, 4, y + 2, 0.8f, 0.8f, 0.8f, 1.0f, 1.0f, C2D_Align_Left, C2D_Align_Top);
    C2D_FinishText(&label_text);
    
    C2D_RGBAFill(x, y, w, h, editing ? 0.1f, 0.3f, 0.8f, 1.0f : 0.2f, 0.2f, 0.2f, 1.0f);
    
    C2D_Text value_text;
    C2D_PrepareText(&value_text, font, 0.6f, value, 1, NULL, 1.0f, 1.0f);
    C2D_DrawText(&value_text, font, 0.6f, x + 2, y + 2, 0.8f, 0.8f, 0.8f, 1.0f, 1.0f, C2D_Align_Left, C2D_Align_Top);
    C2D_FinishText(&value_text);
}

static void drawSlider(const char* label, int value, int min, int max, int x, int y, int w) {
    C2D_Text text;
    C2D_TextFont font = C2D_Font_Regular;
    C2D_PrepareText(&text, font, 0.6f, label, 1, NULL, 1.0f, 1.0f);
    C2D_DrawText(&text, font, 0.6f, 4, y + 4, 0.8f, 0.8f, 0.8f, 1.0f, 1.0f, C2D_Align_Left, C2D_Align_Top);
    C2D_FinishText(&text);
    
    int sliderX = x + 120;
    int sliderW = w - 130;
    C2D_RGBAFill(sliderX, y + 6, sliderW, 12, 0.15f, 0.15f, 0.15f, 1.0f);
    
    float ratio = 0.0f;
    if (max != min)
        ratio = (float)(value - min) / (float)(max - min);
    int knobX = sliderX + (int)(sliderW * ratio);
    C2D_RGBAFill(knobX - 3, y + 3, 6, 18, 0.2f, 0.6f, 1.0f, 1.0f);
    
    char valStr[32];
    snprintf(valStr, sizeof(valStr), "%d", value);
    C2D_Text valText;
    C2D_PrepareText(&valText, font, 0.6f, valStr, 1, NULL, 1.0f, 1.0f);
    C2D_DrawText(&valText, font, 0.6f, sliderX + sliderW + 5, y + 4, 0.8f, 0.8f, 0.8f, 1.0f, 1.0f, C2D_Align_Left, C2D_Align_Top);
    C2D_FinishText(&valText);
}

static void drawCategoryTabs() {
    int tabWidth = PANEL_WIDTH / numCategories;
    for (int i = 0; i < numCategories; i++) {
        int x = PANEL_X + i * tabWidth;
        bool selected = ((int)i == (int)currentCategory);
        drawButton(categoryTabs[i], x, 4, tabWidth - 1, 24, selected);
    }
}

static void drawGeneralSettings() {
    auto g = Settings::getCategoryOptions(currentCategory);
    int y = OPTION_START_Y;
    
    for (size_t i = 0; i < g.size(); i++) {
        const auto& opt = g[i];
        bool selected = (i == selectedOption);
        
        if (opt.isTextInput) {
            drawTextBox(opt.label.c_str(), opt.textValue.c_str(), 
                       TEXTBOX_X, y + 2, TEXTBOX_WIDTH, TEXTBOX_HEIGHT,
                       textEditMode && textEditOption == i);
        } else {
            char buf[64];
            snprintf(buf, sizeof(buf), "%s: %s", opt.label.c_str(), 
                    opt.value ? "ON" : "OFF");
            drawButton(buf, PANEL_X, y, PANEL_WIDTH, BUTTON_HEIGHT, selected);
        }
        y += BUTTON_HEIGHT + BUTTON_MARGIN;
    }
}

static void drawAssetsSettings() {
    auto opts = Settings::getCategoryOptions(currentCategory);
    int y = OPTION_START_Y;
    
    for (size_t i = 0; i < opts.size(); i++) {
        auto& opt = opts[i];
        bool selected = (i == selectedOption);
        
        if (i == 3) {  // Clear Asset Cache button
            char btnText[64];
            if (cacheCleared) {
                snprintf(btnText, sizeof(btnText), "Cache Cleared!");
            } else {
                snprintf(btnText, sizeof(btnText), "Clear Asset Cache");
            }
            drawButton(btnText, CLEAR_CACHE_X, y, CLEAR_CACHE_WIDTH, CLEAR_CACHE_HEIGHT, selected);
        } else {
            char buf[64];
            snprintf(buf, sizeof(buf), "%s: %s", opt.label.c_str(), 
                    opt.value ? "ON" : "OFF");
            drawButton(buf, PANEL_X, y, PANEL_WIDTH, BUTTON_HEIGHT, selected);
        }
        y += BUTTON_HEIGHT + BUTTON_MARGIN;
    }
    
    // Show cache info
    char cacheInfo[128];
    snprintf(cacheInfo, sizeof(cacheInfo), "Cache: sdmc:/OpenBLOX/shareddata/assets/");
    C2D_Text infoText;
    C2D_TextFont font = C2D_Font_Regular;
    C2D_PrepareText(&infoText, font, 0.5f, cacheInfo, 1, NULL, 0.7f, 0.7f);
    C2D_DrawText(&infoText, font, 0.5f, PANEL_X, y + 8, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f, C2D_Align_Left, C2D_Align_Top);
    C2D_FinishText(&infoText);
}

static void drawGraphicsSettings() {
    auto opts = Settings::getCategoryOptions(currentCategory);
    int y = OPTION_START_Y;

    // Draw year preset buttons in a 2x3 grid
    const char* yearBtnLabels[] = { "2012", "2015", "2016", "2019", "2022" };
    int currentYear = (int)Settings::get().general.contentYear;
    for (int i = 0; i < 5; i++) {
        int col = i % 3;
        int row = i / 3;
        int x = PANEL_X + col * 101;
        int yPos = y + row * (BUTTON_HEIGHT + BUTTON_MARGIN);
        bool selected = (i == selectedOption) || (i == currentYear);
        drawButton(yearBtnLabels[i], x, yPos, 99, BUTTON_HEIGHT, selected);
    }
    y += 2 * (BUTTON_HEIGHT + BUTTON_MARGIN) + 8;

    // Draw the rest of the graphics options (starting from index 6)
    for (size_t i = 6; i < opts.size(); i++) {
        auto& opt = opts[i];
        bool selected = (i == selectedOption);

        if (opt.isDropdown) {
            char buf[64];
            int labelIdx = std::clamp(opt.value, 0, (int)opt.dropdownLabels.size() - 1);
            snprintf(buf, sizeof(buf), "%s: %s", opt.label.c_str(), opt.dropdownLabels[labelIdx].c_str());
            drawButton(buf, PANEL_X, y, PANEL_WIDTH, BUTTON_HEIGHT, selected);
        } else if (opt.isToggle) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%s: %s", opt.label.c_str(), opt.value ? "ON" : "OFF");
            drawButton(buf, PANEL_X, y, PANEL_WIDTH, BUTTON_HEIGHT, selected);
        } else {
            drawSlider(opt.label.c_str(), opt.value, opt.minValue, opt.maxValue, PANEL_X, y, PANEL_WIDTH);
        }
        y += BUTTON_HEIGHT + BUTTON_MARGIN;
    }
}

// File selector state
static bool fileSelectorActive = false;
static std::vector<std::string> soundFontFiles;
static std::string selectedSoundFontPath;

// Scan for .sf2 files in the soundfonts directory
static void scanSoundFonts() {
    soundFontFiles.clear();
    std::string soundfontDir = "sdmc:/OpenBLOX/shareddata/soundfonts/";
    
    // Create directory if it doesn't exist
    mkdir(soundfontDir.c_str(), 0755);
    
    DIR* dir = opendir(soundfontDir.c_str());
    if (!dir) return;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) continue;
        std::string name = entry->d_name;
        // Filter for .sf2 files (case-insensitive)
        if (name.size() >= 4) {
            std::string ext = name.substr(name.size() - 4);
            if (ext == ".sf2" || ext == ".SF2") {
                soundFontFiles.push_back(soundfontDir + name);
            }
        }
    }
    closedir(dir);
}

static void drawSoundSettings() {
    auto opts = Settings::getCategoryOptions(currentCategory);
    int y = OPTION_START_Y;
    C2D_TextFont font = C2D_Font_Regular;

    for (size_t i = 0; i < opts.size(); i++) {
        const auto& opt = opts[i];
        bool selected = (i == selectedOption);

        if (opt.isDropdown) {
            char buf[64];
            int labelIdx = std::clamp(opt.value, 0, (int)opt.dropdownLabels.size() - 1);
            snprintf(buf, sizeof(buf), "%s: %s", opt.label.c_str(), opt.dropdownLabels[labelIdx].c_str());
            drawButton(buf, PANEL_X, y, PANEL_WIDTH, BUTTON_HEIGHT, selected);
        } else if (opt.isTextInput) {
            // Custom SoundFont text input field with Browse button
            drawTextBox(opt.label.c_str(), opt.textValue.c_str(),
                       TEXTBOX_X, y + 2, TEXTBOX_WIDTH - 60, TEXTBOX_HEIGHT,
                       textEditMode && textEditOption == i);
            // Browse button
            char browseBtn[16];
            snprintf(browseBtn, sizeof(browseBtn), "Browse...");
            drawButton(browseBtn, TEXTBOX_X + TEXTBOX_WIDTH - 58, y + 2, 56, TEXTBOX_HEIGHT, false);
            
            // Green checkmark indicator if soundfont is loaded
            if (opt.textValue.empty() == false && AudioManager::isCustomSoundFontLoaded()) {
                C2D_RGBAFill(TEXTBOX_X + TEXTBOX_WIDTH - 8, y + 2, 6, 6, 0.0f, 1.0f, 0.0f, 1.0f);
            }
            
            // Show current soundfont status
            char statusText[64];
            if (opt.textValue.empty()) {
                snprintf(statusText, sizeof(statusText), "Status: Using default");
            } else if (AudioManager::isCustomSoundFontLoaded()) {
                snprintf(statusText, sizeof(statusText), "Status: Loaded");
            } else {
                snprintf(statusText, sizeof(statusText), "Status: Not loaded");
            }
            C2D_Text status;
            C2D_PrepareText(&status, font, 0.4f, statusText, 1, NULL, 1.0f, 1.0f);
            C2D_DrawText(&status, font, 0.4f, PANEL_X, y + BUTTON_HEIGHT + 2, 
                        0.7f, 0.7f, 0.7f, 1.0f, 1.0f, C2D_Align_Left, C2D_Align_Top);
            C2D_FinishText(&status);
            y += 12;
        } else if (opt.isToggle) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%s: %s", opt.label.c_str(), opt.value ? "ON" : "OFF");
            drawButton(buf, PANEL_X, y, PANEL_WIDTH, BUTTON_HEIGHT, selected);
        } else {
            drawSlider(opt.label.c_str(), opt.value, opt.minValue, opt.maxValue, PANEL_X, y, PANEL_WIDTH);
        }
        y += BUTTON_HEIGHT + BUTTON_MARGIN;

        // Add note below the Soundfont Interpolation dropdown (index 5)
        if (i == 5) {
            C2D_Text noteText;
            C2D_PrepareText(&noteText, font, 0.45f, "Note: Only applies to MIDI/SoundFont playback,\nnot libxmp module playback.", 1, NULL, 1.0f, 0.7f);
            C2D_DrawText(&noteText, font, 0.45f, PANEL_X + 2, y, 0.6f, 0.6f, 0.5f, 1.0f, 1.0f, C2D_Align_Left, C2D_Align_Top);
            C2D_FinishText(&noteText);
            y += 16;
        }
    }
}

static void handleTextInput() {
    if (!textEditMode || textEditOption < 0) return;
    
    // Handle keyboard input for text editing
    char prevBuffer[256];
    strncpy(prevBuffer, textBuffer, sizeof(prevBuffer));
    
    // Check for key presses
    if (hidKeysDown() & KEY_A) {
        // Confirm and save
        Settings::setOptionTextValue(currentCategory, textEditOption, textBuffer);
        Settings::save("sdmc:/OpenBLOX/settings.sav");
        textEditMode = false;
        textEditOption = -1;
    } else if (hidKeysDown() & KEY_B) {
        // Cancel
        textEditMode = false;
        textEditOption = -1;
    }
    
    // Arrow keys for cursor movement
    if (hidKeysHeld() & KEY_RIGHT) {
        textCursor = std::min(textCursor + 1, (int)strlen(textBuffer));
    } else if (hidKeysHeld() & KEY_LEFT) {
        textCursor = std::max(textCursor - 1, 0);
    }
}

static void handleTouchInput() {
    touchPosition touch;
    hidTouchRead(&touch);
    
    if (hidKeysHeld() & KEY_TOUCH) {
        touchX = touch.px;
        touchY = touch.py;
        
        bool touchJustPressed = (hidKeysDown() & KEY_TOUCH);
        if (touchDown && touchJustPressed) {
            // Check category tabs
            int tabWidth = PANEL_WIDTH / numCategories;
            if (touchY < 28) {
                for (int i = 0; i < numCategories; i++) {
                    int x = PANEL_X + i * tabWidth;
                    if (touchX >= x && touchX < x + tabWidth) {
                        currentCategory = (SettingsCategory)i;
                        selectedOption = 0;
                        scrollOffset = 0;
                        return;
                    }
                }
            }
            
            // Check option buttons
            int y = OPTION_START_Y;
            auto opts = Settings::getCategoryOptions(currentCategory);
            if (currentCategory == SettingsCategory::Graphics) {
                const char* yearBtnLabels[] = { "2012", "2015", "2016", "2019", "2022" };
                for (int i = 0; i < 5; i++) {
                    int col = i % 3;
                    int row = i / 3;
                    int btnX = PANEL_X + col * 101;
                    int btnY = y + row * (BUTTON_HEIGHT + BUTTON_MARGIN);
                    if (touchX >= btnX && touchX < btnX + 99 && touchY >= btnY && touchY < btnY + BUTTON_HEIGHT) {
                        Settings::applyYearPreset((ContentYear)i);
                        Settings::save("sdmc:/OpenBLOX/settings.sav");
                        selectedOption = i;
                        return;
                    }
                }
                // Continue to the rest of the graphics options (skip first 6)
                y += 2 * (BUTTON_HEIGHT + BUTTON_MARGIN) + 8;
                for (size_t i = 6; i < opts.size(); i++) {
                    if (touchY >= y && touchY < y + BUTTON_HEIGHT) {
                        if (touchX >= PANEL_X && touchX <= PANEL_X + PANEL_WIDTH) {
                            auto& opt = opts[i];
                            if (opt.isDropdown) {
                                selectedOption = i;
                            } else if (opt.isToggle) {
                                int newValue = opt.value ? 0 : 1;
                                Settings::setOptionValue(currentCategory, i, newValue);
                                selectedOption = i;
                            } else {
                                selectedOption = i;
                            }
                            return;
                        }
                    }
                    y += BUTTON_HEIGHT + BUTTON_MARGIN;
                }
                return;
            }

            // Special handling for Presets category year preset buttons
            if (currentCategory == SettingsCategory::Presets) {
                const char* yearBtnLabels[] = { "2012", "2015", "2016", "2019", "2022" };
                for (int i = 0; i < 5; i++) {
                    int col = i % 3;
                    int row = i / 3;
                    int btnX = PANEL_X + col * 101;
                    int btnY = y + row * (BUTTON_HEIGHT + BUTTON_MARGIN);
                    if (touchX >= btnX && touchX < btnX + 99 && touchY >= btnY && touchY < btnY + BUTTON_HEIGHT) {
                        Settings::applyYearPreset((ContentYear)i);
                        Settings::save("sdmc:/OpenBLOX/settings.sav");
                        selectedOption = i;
                        return;
                    }
                }
                // Performance preset dropdown (index 6)
                y += 2 * (BUTTON_HEIGHT + BUTTON_MARGIN) + 8;
                for (size_t i = 6; i < opts.size(); i++) {
                    if (touchY >= y && touchY < y + BUTTON_HEIGHT) {
                        if (touchX >= PANEL_X && touchX <= PANEL_X + PANEL_WIDTH) {
                            auto& opt = opts[i];
                            if (opt.isDropdown) {
                                selectedOption = i;
                            } else if (opt.isToggle) {
                                int newValue = opt.value ? 0 : 1;
                                Settings::setOptionValue(currentCategory, i, newValue);
                                selectedOption = i;
                            } else {
                                selectedOption = i;
                            }
                            return;
                        }
                    }
                    y += BUTTON_HEIGHT + BUTTON_MARGIN;
                }
                return;
            }

            // Special handling for Sound category Browse button
            if (currentCategory == SettingsCategory::Sound) {
                int soundY = OPTION_START_Y;
                for (size_t i = 0; i < opts.size(); i++) {
                    if (touchY >= soundY && touchY < soundY + BUTTON_HEIGHT) {
                        auto& opt = opts[i];
                        if (opt.isTextInput) {
                            // Check if touch is on the Browse button (right side)
                            int browseBtnX = TEXTBOX_X + TEXTBOX_WIDTH - 58;
                            if (touchX >= browseBtnX && touchX < browseBtnX + 56 &&
                                touchY >= soundY + 2 && touchY < soundY + 2 + TEXTBOX_HEIGHT) {
                                // Open SoundFont file selector
                                scanSoundFonts();
                                if (!soundFontFiles.empty()) {
                                    // Get display names (just filenames)
                                    std::vector<std::string> displayNames;
                                    for (const auto& path : soundFontFiles) {
                                        size_t pos = path.find_last_of('/');
                                        displayNames.push_back(pos != std::string::npos ? path.substr(pos + 1) : path);
                                    }
                                    int sel = UIFileSelector::selectFromList(displayNames);
                                    if (sel >= 0 && sel < (int)soundFontFiles.size()) {
                                        std::string selectedPath = soundFontFiles[sel];
                                        Settings::setOptionTextValue(currentCategory, i, selectedPath);
                                        Settings::save("sdmc:/OpenBLOX/settings.sav");
                                        AudioManager::loadCustomSoundFont(selectedPath);
                                        printf("[UI] SoundFont loaded: %s\n", selectedPath.c_str());
                                    }
                                } else {
                                    printf("[UI] No .sf2 files found in soundfonts directory\n");
                                }
                                return;
                            }
                            // Touch on text input field - start editing
                            textEditMode = true;
                            textEditOption = i;
                            strncpy(textBuffer, opt.textValue.c_str(), sizeof(textBuffer) - 1);
                            textCursor = strlen(textBuffer);
                            selectedOption = i;
                            return;
                        } else if (opt.isToggle) {
                            int newValue = opt.value ? 0 : 1;
                            Settings::setOptionValue(currentCategory, i, newValue);
                            selectedOption = i;
                            return;
                        } else {
                            selectedOption = i;
                            return;
                        }
                    }
                    soundY += BUTTON_HEIGHT + BUTTON_MARGIN;
                    // Add extra offset after interpolation dropdown (index 5)
                    if (i == 5) soundY += 16;
                    // Add extra offset for Custom SoundFont status text (index 6)
                    if (i == 6) soundY += 12;
                }
                return;
            }

            for (size_t i = 0; i < opts.size(); i++) {
                if (touchY >= y && touchY < y + BUTTON_HEIGHT) {
                    if (touchX >= PANEL_X && touchX <= PANEL_X + PANEL_WIDTH) {
                        auto& opt = opts[i];
                        
                        if (opt.isTextInput) {
                            // Start text editing
                            textEditMode = true;
                            textEditOption = i;
                            strncpy(textBuffer, opt.textValue.c_str(), sizeof(textBuffer) - 1);
                            textCursor = strlen(textBuffer);
                            selectedOption = i;
                            return;
                        } else if (opt.isToggle) {
                            // Toggle option
                            int newValue = opt.value ? 0 : 1;
                            Settings::setOptionValue(currentCategory, i, newValue);
                            
                            // Special case: Clear Asset Cache button
                            if (currentCategory == SettingsCategory::Assets && i == 3 && newValue == 1) {
                                AssetLoader::clearAllCache();
                                printf("[UI] Asset cache cleared\n");
                                cacheCleared = true;
                                selectedOption = -1;  // Reset selection after clearing
                            } else {
                                selectedOption = i;
                            }
                        } else {
                            selectedOption = i;
                        }
                    }
                }
                y += BUTTON_HEIGHT + BUTTON_MARGIN;
            }
        }
        
        touchDown = true;
    } else {
        touchDown = false;
    }
}

void uiSettingsInit() {
    currentCategory = SettingsCategory::General;
    selectedOption = 0;
    scrollOffset = 0;
    textEditMode = false;
    textEditOption = -1;
    memset(textBuffer, 0, sizeof(textBuffer));
    textCursor = 0;
    cacheCleared = false;
}

void uiSettingsUpdate() {
    handleTouchInput();
    handleTextInput();
    
    // Handle D-pad navigation
    if (!textEditMode) {
        auto opts = Settings::getCategoryOptions(currentCategory);
        
        if (hidKeysDown() & KEY_UP) {
            selectedOption = std::max(selectedOption - 1, 0);
        } else if (hidKeysDown() & KEY_DOWN) {
            selectedOption = std::min(selectedOption + 1, (int)opts.size() - 1);
        } else if (hidKeysDown() & KEY_A) {
            if (selectedOption >= 0 && selectedOption < (int)opts.size()) {
                auto& opt = opts[selectedOption];
                
                if (opt.isTextInput) {
                    textEditMode = true;
                    textEditOption = selectedOption;
                    strncpy(textBuffer, opt.textValue.c_str(), sizeof(textBuffer) - 1);
                    textCursor = strlen(textBuffer);
                } else if (opt.isToggle) {
                    int newValue = opt.value ? 0 : 1;
                    Settings::setOptionValue(currentCategory, selectedOption, newValue);
                    
                    if (currentCategory == SettingsCategory::Assets && selectedOption == 3 && newValue == 1) {
                        AssetLoader::clearAllCache();
                        printf("[UI] Asset cache cleared\n");
                        cacheCleared = true;
                    }
                }
            }
        } else if (hidKeysDown() & KEY_RIGHT) {
            currentCategory = (SettingsCategory)(((int)currentCategory + 1) % numCategories);
            selectedOption = 0;
        } else if (hidKeysDown() & KEY_LEFT) {
            currentCategory = (SettingsCategory)(((int)currentCategory - 1 + numCategories) % numCategories);
            selectedOption = 0;
        }
    }
}

void uiSettingsDraw() {
    C2D_RGBAFill(0, 0, 320, 240, 0.1f, 0.1f, 0.1f, 1.0f);
    
    // Title
    C2D_Text title;
    C2D_TextFont font = C2D_Font_Regular;
    C2D_PrepareText(&title, font, 0.8f, "Settings", 1, NULL, 1.0f, 1.0f);
    C2D_DrawText(&title, font, 0.8f, 160, 2, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f, C2D_Align_Center, C2D_Align_Top);
    C2D_FinishText(&title);
    
    drawCategoryTabs();
    
    switch (currentCategory) {
        case SettingsCategory::General:
            drawGeneralSettings();
            break;
        case SettingsCategory::Graphics:
            drawGraphicsSettings();
            break;
        case SettingsCategory::Sound:
            drawSoundSettings();
            break;
        case SettingsCategory::Assets:
            drawAssetsSettings();
            break;
        case SettingsCategory::Presets:
            // Year preset buttons (2x3 grid)
            {
                const char* yearBtnLabels[] = { "2012", "2015", "2016", "2019", "2022" };
                int y = OPTION_START_Y;
                auto opts = Settings::getCategoryOptions(currentCategory);
                for (int i = 0; i < 5; i++) {
                    int col = i % 3;
                    int row = i / 3;
                    int btnX = PANEL_X + col * 101;
                    int btnY = y + row * (BUTTON_HEIGHT + BUTTON_MARGIN);
                    bool selected = (i == selectedOption);
                    drawButton(yearBtnLabels[i], btnX, btnY, 99, BUTTON_HEIGHT, selected);
                }
                y += 2 * (BUTTON_HEIGHT + BUTTON_MARGIN) + 8;
                for (size_t i = 6; i < opts.size(); i++) {
                    auto& opt = opts[i];
                    bool selected = (i == selectedOption);
                    if (opt.isDropdown) {
                        char buf[64];
                        int labelIdx = std::clamp(opt.value, 0, (int)opt.dropdownLabels.size() - 1);
                        snprintf(buf, sizeof(buf), "%s: %s", opt.label.c_str(), opt.dropdownLabels[labelIdx].c_str());
                        drawButton(buf, PANEL_X, y, PANEL_WIDTH, BUTTON_HEIGHT, selected);
                    }
                    y += BUTTON_HEIGHT + BUTTON_MARGIN;
                }
            }
            break;
        default:
            // Other categories use simple list
            auto opts = Settings::getCategoryOptions(currentCategory);
            int y = OPTION_START_Y;
            for (size_t i = 0; i < opts.size(); i++) {
                const auto& opt = opts[i];
                bool selected = (i == selectedOption);

                if (opt.isDropdown) {
                    char buf[64];
                    int labelIdx = std::clamp(opt.value, 0, (int)opt.dropdownLabels.size() - 1);
                    snprintf(buf, sizeof(buf), "%s: %s", opt.label.c_str(), opt.dropdownLabels[labelIdx].c_str());
                    drawButton(buf, PANEL_X, y, PANEL_WIDTH, BUTTON_HEIGHT, selected);
                } else if (opt.isToggle) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "%s: %s", opt.label.c_str(), opt.value ? "ON" : "OFF");
                    drawButton(buf, PANEL_X, y, PANEL_WIDTH, BUTTON_HEIGHT, selected);
                } else {
                    drawSlider(opt.label.c_str(), opt.value, opt.minValue, opt.maxValue, PANEL_X, y, PANEL_WIDTH);
                }
                y += BUTTON_HEIGHT + BUTTON_MARGIN;
            }
            break;
    }
    
    // Draw text edit overlay
    if (textEditMode && textEditOption >= 0) {
        C2D_RGBAFill(0, 0, 320, 240, 0.0f, 0.0f, 0.0f, 0.5f);
        
        char label[128];
        snprintf(label, sizeof(label), "Edit: %s", 
                Settings::getCategoryOptions(currentCategory)[textEditOption].label.c_str());
        
        C2D_Text editLabel;
        C2D_PrepareText(&editLabel, font, 0.6f, label, 1, NULL, 1.0f, 1.0f);
        C2D_DrawText(&editLabel, font, 0.6f, 160, 100, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f, C2D_Align_Center, C2D_Align_Top);
        C2D_FinishText(&editLabel);
        
        C2D_RGBAFill(40, 120, 240, 24, 0.2f, 0.2f, 0.2f, 1.0f);
        
        C2D_Text editText;
        C2D_PrepareText(&editText, font, 0.6f, textBuffer, 1, NULL, 1.0f, 1.0f);
        C2D_DrawText(&editText, font, 0.6f, 42, 122, 0.8f, 0.8f, 0.8f, 1.0f, 1.0f, C2D_Align_Left, C2D_Align_Top);
        C2D_FinishText(&editText);
        
        C2D_Text helpText;
        C2D_PrepareText(&helpText, font, 0.5f, "A=Save B=Cancel", 1, NULL, 0.7f, 0.7f);
        C2D_DrawText(&helpText, font, 0.5f, 160, 150, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f, C2D_Align_Center, C2D_Align_Top);
        C2D_FinishText(&helpText);
    }
    
    // Footer
    C2D_Text footer;
    C2D_PrepareText(&footer, font, 0.5f, "D-Pad: Navigate  A: Select  B: Back", 1, NULL, 0.6f, 0.6f);
    C2D_DrawText(&footer, font, 0.5f, 160, 232, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f, C2D_Align_Center, C2D_Align_Top);
    C2D_FinishText(&footer);
}

bool uiSettingsIsActive() {
    return textEditMode || currentCategory != SettingsCategory::About;
}