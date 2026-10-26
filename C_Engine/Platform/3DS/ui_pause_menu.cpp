#include "ui_pause_menu.h"
#include "font_manager.h"
#include "citro_renderer.h"
#include "render_pipeline.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <unistd.h>
#include <3ds.h>

namespace UIPauseMenu {

    static PauseState state = PauseState::Inactive;
    static Settings::SettingsCategory selectedCategory = Settings::SettingsCategory::General;
    static int selectedOptionIndex = 0;
    static int optionScrollOffset = 0;
    static int categoryScrollOffset = 0;
    static int numCategories = (int)Settings::SettingsCategory::Count;
    static const int maxVisibleCategories = 8;
    static const int maxVisibleOptions = 8;

    // Layout constants (bottom screen: 320x240)
    static const int sidebarX = 0;
    static const int sidebarW = 90;
    static const int sidebarItemH = 26;
    static const int contentX = 90;
    static const int contentW = 230;
    static const int optionH = 26;
    static const int headerH = 26;
    static const int footerH = 22;
    static const int scrollbarW = 8;

    // Scrollbar touch state
    static bool scrollbarDragging = false;
    static int scrollbarDragStartY = 0;
    static int scrollbarDragStartOffset = 0;
    static bool scrollbarIsCategory = false;

    // Touch/leave state
    static bool leaveGame = false;

    void init() {
        state = PauseState::Inactive;
        selectedCategory = Settings::SettingsCategory::General;
        selectedOptionIndex = 0;
        optionScrollOffset = 0;
        categoryScrollOffset = 0;
        scrollbarDragging = false;
        scrollbarDragStartY = 0;
        scrollbarDragStartOffset = 0;
        scrollbarIsCategory = false;
    }

    void togglePause() {
        if (state == PauseState::Inactive) {
            state = PauseState::Paused;
        } else if (state == PauseState::Paused) {
            state = PauseState::Inactive;
        }
    }

    PauseState getState() { return state; }

    void setState(PauseState s) {
        if (s != state) {
            state = s;
            if (s == PauseState::Settings) {
                selectedCategory = Settings::SettingsCategory::General;
                selectedOptionIndex = 0;
                optionScrollOffset = 0;
                categoryScrollOffset = 0;
                scrollbarDragging = false;
            }
        }
    }

    bool isGamePaused() {
        return state != PauseState::Inactive;
    }

    // Draw helper to replace CTR_LVGLite_drawRect (which doesn't exist in libctru)
    static void drawRect(int x, int y, int w, int h, float r, float g, float b, float a) {
        (void)a; // Alpha not used in DrawRect
        DrawRect(x, y, w, h, (int)(r * 255), (int)(g * 255), (int)(b * 255));
    }

    // Draw a scrollbar (vertical)
    static void drawScrollbar(int x, int y, int h, int totalItems, int visibleItems, int scrollOffset,
                              uint8_t selR, uint8_t selG, uint8_t selB) {
        if (totalItems <= visibleItems) return;
        // Track
        drawRect(x, y, scrollbarW, h, 0.15f, 0.15f, 0.15f, 1.0f);
        // Thumb
        int thumbH = (int)((float)visibleItems / totalItems * h);
        if (thumbH < 12) thumbH = 12;
        float thumbY = (float)scrollOffset / (totalItems - visibleItems) * (h - thumbH);
        drawRect(x + 1, y + (int)thumbY, scrollbarW - 2, thumbH,
            selR/255.0f, selG/255.0f, selB/255.0f, 0.6f);
    }

    // Draw a toggle indicator
    static void drawToggle(int x, int y, bool val) {
        int r, g, b;
        if (val) { r = 100; g = 255; b = 100; }
        else { r = 200; g = 100; b = 100; }
        FontManager::drawText(val ? "On" : "Off", x, y, r, g, b, FontSize::Size12);
    }

    // Draw a horizontal bar with label
    static void drawBar(int x, int y, int w, int h, const char* label, int progress, int maxVal) {
        drawRect(x, y, w, h, 0.2f, 0.2f, 0.2f, 1.0f);
        int progW = (maxVal > 0) ? (w * progress / maxVal) : w;
        drawRect(x, y, progW, h, 0.2f, 0.8f, 0.2f, 1.0f);
        FontManager::drawText(label, x + w + 5, y, 200, 200, 200, FontSize::Size12);
    }

    void draw() {
        if (state == PauseState::Inactive) return;

        consoleClear();
        consoleSelect(NULL);

        if (state == PauseState::Paused) {
            iprintf("\n\n   === PAUSED ===\n\n");
            iprintf("  [A] Resume\n");
            iprintf("  [X] Settings\n");
            iprintf("  [B] Leave Game\n");
            return;
        }

        if (state == PauseState::Settings) {
            // === YEAR-THEMED SETTINGS MENU ===
            int contentYear = 2012;
            switch(Settings::get().general.contentYear) {
                case Settings::ContentYear::Year2007: contentYear = 2007; break;
                case Settings::ContentYear::Year2011: contentYear = 2011; break;
                case Settings::ContentYear::Year2012: contentYear = 2012; break;
                case Settings::ContentYear::Year2015: contentYear = 2015; break;
                case Settings::ContentYear::Year2016: contentYear = 2016; break;
                case Settings::ContentYear::Year2019: contentYear = 2019; break;
                case Settings::ContentYear::Year2022: contentYear = 2022; break;
            }

            uint8_t bgR, bgG, bgB;
            uint8_t fgR, fgG, fgB;
            uint8_t selR, selG, selB;
            uint8_t dimR, dimG, dimB;
            uint8_t barR, barG, barB;

            if (contentYear <= 2012) {
                // 2012: Classic dark
                bgR = 40; bgG = 40; bgB = 40;
                fgR = 220; fgG = 220; fgB = 200;
                selR = 255; selG = 255; selB = 100;
                dimR = 150; dimG = 150; dimB = 130;
                barR = 80; barG = 80; barB = 80;
            } else if (contentYear <= 2016) {
                // 2015-2016: Blue-tinted
                bgR = 30; bgG = 40; bgB = 55;
                fgR = 200; fgG = 220; fgB = 255;
                selR = 100; selG = 180; selB = 255;
                dimR = 130; dimG = 150; dimB = 180;
                barR = 60; barG = 70; barB = 90;
            } else {
                // 2019+: Modern dark
                bgR = 25; bgG = 25; bgB = 30;
                fgR = 220; fgG = 220; fgB = 230;
                selR = 200; selG = 130; selB = 255;
                dimR = 140; dimG = 140; dimB = 160;
                barR = 50; barG = 50; barB = 60;
            }

            // === BACKGROUND ===
            RenderPipeline::drawRect(0, 0, 320, 240, bgR/255.0f, bgG/255.0f, bgB/255.0f, 1.0f);

            // === CATEGORY SIDEBAR (Left column) ===
            // Draw sidebar background
            RenderPipeline::drawRect(0, 0, sidebarX + sidebarW, 240,
                barR/255.0f, barG/255.0f, barB/255.0f, 1.0f);

            int visibleCats = std::min(maxVisibleCategories, numCategories);
            for (int i = 0; i < visibleCats; i++) {
                int catIdx = categoryScrollOffset + i;
                if (catIdx >= numCategories) break;

                Settings::SettingsCategory cat = (Settings::SettingsCategory)catIdx;
                const char* name = Settings::getCategoryName(cat);
                int y = 4 + i * sidebarItemH;

                if (cat == selectedCategory) {
                    // Highlight selected category background
                    RenderPipeline::drawRect(2, y - 1, sidebarW - 4, sidebarItemH - 2,
                        selR/255.0f, selG/255.0f, selB/255.0f, 0.3f);
                    FontManager::drawText(name, 12, y, selR, selG, selB, FontSize::Size12);
                } else {
                    FontManager::drawText(name, 12, y, dimR, dimG, dimB, FontSize::Size12);
                }
            }

            // Category scrollbar (right edge of sidebar)
            if (numCategories > maxVisibleCategories) {
                int barY = 4;
                int barH = 240 - 8;
                int thumbH = std::max(12, (int)((float)maxVisibleCategories / numCategories * barH));
                int thumbY = barY + (int)((float)categoryScrollOffset / (numCategories - maxVisibleCategories) * (barH - thumbH));
                // Track
                RenderPipeline::drawRect(sidebarW - scrollbarW, barY, scrollbarW, barH,
                    0.1f, 0.1f, 0.1f, 0.5f);
                // Thumb
                RenderPipeline::drawRect(sidebarW - scrollbarW + 1, thumbY, scrollbarW - 2, thumbH,
                    dimR/255.0f, dimG/255.0f, dimB/255.0f, 0.8f);
            }

            // === CONTENT AREA (Right column) ===
            // Header bar
            RenderPipeline::drawRect(contentX, 0, contentW, headerH,
                barR/255.0f, barG/255.0f, barB/255.0f, 1.0f);
            FontManager::drawText(Settings::getCategoryName(selectedCategory), contentX + 8, 8, selR, selG, selB, FontSize::Size16);

            auto opts = Settings::getCategoryOptions(selectedCategory);
            int totalOpts = (int)opts.size();
            int visibleOpts = std::min(maxVisibleOptions, totalOpts);
            int optionsAreaY = headerH;
            int optionsAreaH = 240 - optionsAreaY - footerH;

            // Options background
            RenderPipeline::drawRect(contentX, optionsAreaY, contentW, optionsAreaH,
                bgR/255.0f, bgG/255.0f, bgB/255.0f, 1.0f);

            for (int i = 0; i < visibleOpts; i++) {
                int optIdx = optionScrollOffset + i;
                if (optIdx >= totalOpts) break;

                const Settings::SettingsOption& opt = opts[optIdx];
                int y = optionsAreaY + 4 + i * optionH;

                // Selected option highlight
                if (optIdx == selectedOptionIndex) {
                    RenderPipeline::drawRect(contentX + 2, y - 1, contentW - 4, optionH - 2,
                        selR/255.0f, selG/255.0f, selB/255.0f, 0.15f);
                    FontManager::drawText(">", contentX + 4, y, selR, selG, selB, FontSize::Size12);
                } else {
                    FontManager::drawText(" ", contentX + 4, y, dimR, dimG, dimB, FontSize::Size12);
                }

                // Label
                char labelBuf[60];
                snprintf(labelBuf, sizeof(labelBuf), "%s:", opt.label.c_str());
                uint8_t labelR = (optIdx == selectedOptionIndex) ? fgR : dimR;
                uint8_t labelG = (optIdx == selectedOptionIndex) ? fgG : dimG;
                uint8_t labelB = (optIdx == selectedOptionIndex) ? fgB : dimB;
                FontManager::drawText(labelBuf, contentX + 16, y, labelR, labelG, labelB, FontSize::Size12);

                // Value
                int valX = contentX + 140;
                if (opt.isToggle) {
                    bool val = opt.value != 0;
                    int tr = val ? 100 : 180;
                    int tg = val ? 255 : 80;
                    int tb = val ? 100 : 80;
                    FontManager::drawText(val ? "On" : "Off", valX, y, tr, tg, tb, FontSize::Size12);
                } else if (opt.isDropdown && !opt.dropdownLabels.empty()) {
                    int idx = std::clamp(opt.value, 0, (int)opt.dropdownLabels.size() - 1);
                    FontManager::drawText(opt.dropdownLabels[idx].c_str(), valX, y, fgR, fgG, fgB, FontSize::Size12);
                } else {
                    char valText[32];
                    snprintf(valText, sizeof(valText), "%d%s", opt.value, opt.unit.c_str());
                    FontManager::drawText(valText, valX, y, fgR, fgG, fgB, FontSize::Size12);
                }
            }

            // Options scrollbar (right edge of content)
            if (totalOpts > maxVisibleOptions) {
                int barY = optionsAreaY;
                int barH = optionsAreaH;
                int thumbH = std::max(12, (int)((float)maxVisibleOptions / totalOpts * barH));
                int thumbY = barY + (int)((float)optionScrollOffset / (totalOpts - maxVisibleOptions) * (barH - thumbH));
                // Track
                RenderPipeline::drawRect(contentX + contentW - scrollbarW, barY, scrollbarW, barH,
                    0.1f, 0.1f, 0.1f, 0.5f);
                // Thumb
                RenderPipeline::drawRect(contentX + contentW - scrollbarW + 1, thumbY, scrollbarW - 2, thumbH,
                    selR/255.0f, selG/255.0f, selB/255.0f, 0.6f);
            }

            // === FOOTER ===
            RenderPipeline::drawRect(contentX, 240 - footerH, contentW, footerH,
                barR/255.0f, barG/255.0f, barB/255.0f, 1.0f);
            FontManager::drawText("[A] Toggle  [L/R] Adjust  [Left] Cat  [B] Back", contentX + 4, 240 - footerH + 6, dimR, dimG, dimB, FontSize::Size12);
        }
    }

    void processInput() {
        if (state == PauseState::Paused) {
            u32 kDown = hidKeysDown();
            if (kDown & KEY_A) {
                // Resume
                setState(PauseState::Inactive);
            } else if (kDown & KEY_X) {
                // Settings
                setState(PauseState::Settings);
            } else if (kDown & KEY_B) {
                // Leave game - set global flag for main loop
                leaveGame = true;
                setState(PauseState::Inactive);
            }
            return;
        }

        if (state == PauseState::Settings) {
            // Pre-compute opts/totalOpts for use by all handlers including touch
            auto opts = Settings::getCategoryOptions(selectedCategory);
            int totalOpts = (int)opts.size();

            u32 kDown = hidKeysDown();

            // B to go back and save
            if (kDown & KEY_B) {
                Settings::save("sdmc:/OpenBLOX/settings.ini");
                setState(PauseState::Paused);
                return;
            }

            // UP/DOWN for category selection (left column)
            if (kDown & KEY_UP) {
                // Try to scroll up options first, then categories
                if (selectedOptionIndex > 0) {
                    selectedOptionIndex--;
                }
                if (selectedOptionIndex < optionScrollOffset) {
                    optionScrollOffset = selectedOptionIndex;
                }
                if (selectedOptionIndex < 0) {
                    // Wrap to categories
                    selectedCategory = (Settings::SettingsCategory)((int)selectedCategory - 1);
                    if ((int)selectedCategory < 0) selectedCategory = (Settings::SettingsCategory)(numCategories - 1);
                    selectedOptionIndex = 0;
                    optionScrollOffset = 0;
                    // Adjust category scroll
                    if ((int)selectedCategory < categoryScrollOffset) {
                        categoryScrollOffset = (int)selectedCategory;
                    }
                }
            }
            if (kDown & KEY_DOWN) {
                // (opts/totalOpts already computed above)
                selectedOptionIndex++;
                if (selectedOptionIndex >= totalOpts) {
                    // Wrap to next category
                    selectedCategory = (Settings::SettingsCategory)((int)selectedCategory + 1);
                    if ((int)selectedCategory >= numCategories) selectedCategory = Settings::SettingsCategory::General;
                    selectedOptionIndex = 0;
                    optionScrollOffset = 0;
                    // Adjust category scroll
                    if ((int)selectedCategory >= categoryScrollOffset + maxVisibleCategories) {
                        categoryScrollOffset = (int)selectedCategory - maxVisibleCategories + 1;
                    }
                }
                if (selectedOptionIndex >= optionScrollOffset + maxVisibleOptions) {
                    optionScrollOffset = selectedOptionIndex - maxVisibleOptions + 1;
                }
            }

            // Left arrow: navigate categories (D-pad left)
            if (kDown & KEY_LEFT) {
                selectedCategory = (Settings::SettingsCategory)((int)selectedCategory - 1);
                if ((int)selectedCategory < 0) selectedCategory = (Settings::SettingsCategory)(numCategories - 1);
                selectedOptionIndex = 0;
                optionScrollOffset = 0;
                if ((int)selectedCategory < categoryScrollOffset) {
                    categoryScrollOffset = (int)selectedCategory;
                }
            }

            // Right arrow: switch back to options focus
            if (kDown & KEY_RIGHT) {
                // Just move to next option or stay on options
            }

            // L/R to adjust value
            if (kDown & KEY_L) {
                // Refresh opts if category changed
                opts = Settings::getCategoryOptions(selectedCategory);
                totalOpts = (int)opts.size();
                if (selectedOptionIndex >= 0 && selectedOptionIndex < totalOpts) {
                    Settings::SettingsOption& opt = opts[selectedOptionIndex];
                    opt.value--;
                    if (opt.value < opt.minValue) opt.value = opt.maxValue;
                    Settings::setOptionValue(selectedCategory, selectedOptionIndex, opt.value);
                }
            }
            if (kDown & KEY_R) {
                opts = Settings::getCategoryOptions(selectedCategory);
                totalOpts = (int)opts.size();
                if (selectedOptionIndex >= 0 && selectedOptionIndex < totalOpts) {
                    Settings::SettingsOption& opt = opts[selectedOptionIndex];
                    opt.value++;
                    if (opt.value > opt.maxValue) opt.value = opt.minValue;
                    Settings::setOptionValue(selectedCategory, selectedOptionIndex, opt.value);
                }
            }

            // A to toggle
            if (kDown & KEY_A) {
                opts = Settings::getCategoryOptions(selectedCategory);
                totalOpts = (int)opts.size();
                if (selectedOptionIndex >= 0 && selectedOptionIndex < totalOpts) {
                    Settings::SettingsOption& opt = opts[selectedOptionIndex];
                    if (opt.isToggle) {
                        opt.value = opt.value ? 0 : 1;
                    } else {
                        opt.value++;
                        if (opt.value > opt.maxValue) opt.value = opt.minValue;
                    }
                    Settings::setOptionValue(selectedCategory, selectedOptionIndex, opt.value);
                }
            }

            // === TOUCH SCROLLBAR HANDLING ===
            touchPosition touch;
            hidTouchRead(&touch);
            u32 kHeld = hidKeysHeld();

            if (scrollbarDragging) {
                if (kHeld & KEY_TOUCH) {
                    int deltaY = touch.py - scrollbarDragStartY;
                    if (scrollbarIsCategory) {
                        int maxScroll = std::max(0, numCategories - maxVisibleCategories);
                        float ratio = (float)deltaY / (240 - 24);
                        categoryScrollOffset = std::max(0, std::min(maxScroll,
                            scrollbarDragStartOffset - (int)(ratio * maxScroll)));
                    } else {
                        auto opts = Settings::getCategoryOptions(selectedCategory);
                        int totalOpts = (int)opts.size();
                        int maxScroll = std::max(0, totalOpts - maxVisibleOptions);
                        int optionsAreaH = 240 - headerH - footerH;
                        float ratio = (float)deltaY / optionsAreaH;
                        optionScrollOffset = std::max(0, std::min(maxScroll,
                            scrollbarDragStartOffset - (int)(ratio * maxScroll)));
                    }
                } else {
                    scrollbarDragging = false;
                }
            } else if (kDown & KEY_TOUCH) {
                // Check category scrollbar
                if (touch.px >= sidebarW - scrollbarW && numCategories > maxVisibleCategories) {
                    scrollbarDragging = true;
                    scrollbarIsCategory = true;
                    scrollbarDragStartY = touch.py;
                    scrollbarDragStartOffset = categoryScrollOffset;
                }
                // Check options scrollbar
                else if (touch.px >= contentX + contentW - scrollbarW) {
                    if (totalOpts > maxVisibleOptions) {
                        scrollbarDragging = true;
                        scrollbarIsCategory = false;
                        scrollbarDragStartY = touch.py;
                        scrollbarDragStartOffset = optionScrollOffset;
                    }
                }
                // Touch option directly
                else if (touch.px >= contentX && touch.py >= headerH && touch.py < 240 - footerH) {
                    int optIdx = (touch.py - headerH) / optionH;
                    if (optIdx >= 0 && optIdx < maxVisibleOptions) {
                        int globalIdx = optionScrollOffset + optIdx;
                        if (globalIdx < totalOpts) {
                            selectedOptionIndex = globalIdx;
                            Settings::SettingsOption& opt = opts[globalIdx];
                            if (opt.isToggle) {
                                opt.value = opt.value ? 0 : 1;
                            } else {
                                opt.value++;
                                if (opt.value > opt.maxValue) opt.value = opt.minValue;
                            }
                            Settings::setOptionValue(selectedCategory, globalIdx, opt.value);
                        }
                    }
                }
                // Touch category directly
                else if (touch.px < sidebarW) {
                    int catIdx = touch.py / sidebarItemH;
                    if (catIdx >= 0 && catIdx < numCategories) {
                        selectedCategory = (Settings::SettingsCategory)catIdx;
                        selectedOptionIndex = 0;
                        optionScrollOffset = 0;
                        if (catIdx >= categoryScrollOffset + maxVisibleCategories)
                            categoryScrollOffset = catIdx - maxVisibleCategories + 1;
                        if (catIdx < categoryScrollOffset)
                            categoryScrollOffset = catIdx;
                    }
                }
            }
        }
    }

    void shutdown() {
        setState(PauseState::Inactive);
        Settings::save("sdmc:/OpenBLOX/settings.ini");
    }

}