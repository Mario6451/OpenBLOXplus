#include <stdio.h>
#include <string.h>

#include "./importantIncludes.h"
#include "CoreEngine/debug_logger.h"
#include "Platform/3DS/player_controller.h"
#include "Platform/3DS/ui_main_menu.h"
#include "Platform/3DS/ui_ingame.h"
#include "Platform/3DS/ui_avatar_editor.h"
#include "Platform/3DS/character_editor.h"
#include "Platform/3DS/script_engine.h"
#include "Platform/3DS/settings.h"
#include "Platform/3DS/asset_loader.h"
#include "Platform/3DS/content_system.h"
#include "Platform/3DS/chat_system.h"
#include "Platform/3DS/audio_manager.h"
#include "Platform/3DS/studio_client.h"
#include "Platform/3DS/voice_chat.h"
#include "CoreEngine/icon_loader.h"
#include "Platform/3DS/ui_asset_loader.h"
#include "Platform/3DS/cursor_system.h"

Node *dataModel, *characterModel;
static bool leaveGame = false;
static bool studioMode = false;

int main(int argc, char *argv[])
{
    // Enable printing debug info
    enableDebugConsole(1);
    systemStart();

    if (startEngine) {
        print_message("Start engine done, now loading data.\n");
        currentLanguageCode=0;

        // Print hello and platform
        print_message("%s: %s\n%s: %s\n",
                strings_debug[STRING_CURRENT_LANGUAGE][currentLanguageCode], locale_names[currentLanguageCode],
                strings_debug[STRING_CURRENT_PLATFORM][currentLanguageCode], strings_platform[platformNameLoc][currentLanguageCode]);
        
        print_message("Starting OpenBLOX v0.1.0\n");

        // Mount SD card and initialize settings
        fsInit();
        Settings::init();
        Settings::load("sdmc:/OpenBLOX/settings.sav");
        LOG_INFO("Settings loaded (debug mode=%d, file logging=%d)", 
            Settings::get().debug.debugMode, Settings::get().debug.enableFileLogging);

        // Initialize debug logger if enabled in settings
        DebugLogger::init(Settings::get().debug.enableFileLogging);

        // Initialize icon loader and voice chat
        IconLoader::init();
        VoiceChat::init();
        CursorSystem::init();
        LOG_INFO("IconLoader, VoiceChat, and CursorSystem initialized");

        // Initialize content system
        ContentSystem::init();

        // Initialize UI asset loader with current content year
        UIAssetLoader::init();
        // TODO: UIAssetLoader::setContentYear(Settings::get().general.contentYear);
        UIAssetLoader::reloadAllAssets();
        LOG_INFO("ContentSystem and UIAssetLoader initialized");

        // Main menu loop (supports returning from game or studio)
        while (true) {
            std::string selectedGame = UIMainMenu::runMenu();
            
            // If no game selected (quit), exit
            if (selectedGame.empty()) {
                print_message("No game selected, exiting.\n");
                DebugLogger::shutdown();
                return 0;
            }

            // Check for special menu actions
            if (selectedGame == "__AVATAR__") {
                print_message("Opening Avatar Editor...\n");
                UIAvatarEditor::init();
                UIAvatarEditor::run();
                UIAvatarEditor::shutdown();
                print_message("Exited Avatar Editor.\n");
                continue; // Return to main menu
            }

            if (selectedGame == "__SETTINGS__") {
                print_message("Opening Settings...\n");
                // TODO: Settings::runSettingsUI();
                continue;
            }

            if (selectedGame == "__MULTIPLAYER__") {
                print_message("Opening Multiplayer menu...\n");
                // TODO: UIMainMenu::runMultiplayerMenu();
                continue;
            }

            // Check if Studio mode was selected
            if (selectedGame == "__STUDIO_MODE__") {
                print_message("Entering Studio mode...\n");
                runtimeMode = __STUDIO_MODE;
                
                // Run Studio editor
                UIMainMenu::runStudio(selectedGame);
                
                // After studio exits, check if we want to go back to menu
                print_message("Exited Studio mode.\n");
                StudioClient::shutdown();
                continue; // Return to main menu
            }
            
            print_message("Selected game: %s\n", selectedGame.c_str());

            if (1/*platSingleApp*/) {
                print_message("Loading game...\n");
                // Load game from selected path
                FILE* gameFile = fopen(selectedGame.c_str(), "rb");
                if (gameFile) {
                    dataModel = parseRBXL(gameFile);
                    fclose(gameFile);
                }
                if (!dataModel) {
                    loadFiles(&dataModel, &characterModel, argc, argv);
                }
                print_message("Game loaded.\n");
                
                // Initialize script engine and load game scripts
                ScriptEngine::init();
                ScriptEngine::loadScriptsFromNode(dataModel);
                print_message("Scripts initialized, loaded %d scripts\n", ScriptEngine::getLoadedScriptCount());
            }

            initOpenGL();
            perspectiveModeGL();

            // Initialize in-game UI
            UInGame::init();
            
            // Enable cursor system for in-game mode
            CursorSystem::setEnabled(true);
            
            // Set default player info
            PlayerInfo playerInfo;
            playerInfo.username = "Player";
            playerInfo.health = 100.0f;
            playerInfo.maxHealth = 100.0f;
            UInGame::setPlayerInfo(playerInfo);

            CameraTrackpad trackpad;
            trackpad.deltaX = 0.0f;
            trackpad.deltaY = 0.0f;
            trackpad.active = false;
            trackpad.startX = 0;
            trackpad.startY = 0;

            InGameUIState uiState = InGameUIState::Playing;

            while(isGameRunning && !leaveGame) {
                // compute delta time at start of frame
                deltaTime = deltaTimeCalc();
                
                // Process in-game UI input (handles pause, trackpad, zoom, hotbar)
                UInGame::processInGameInput(trackpad, uiState);
                
                // Only process gameplay input if not paused
                if (uiState == InGameUIState::Playing) {
                    processInput();
                    
                    // Apply camera trackpad to camera rotation
                    if (trackpad.active) {
                        camera_rx += trackpad.deltaY * 2.0f;  // Vertical look
                        camera_ry += trackpad.deltaX * 2.0f;  // Horizontal look
                    }
                    
                    // update player controller with current delta
                    PlayerController::updatePlayer((float)deltaTime);
                }
                
                // Draw top screen (3D game)
                gameRenderLoop();
                TPE_worldStep(&world);
                
                // Update tracker/MIDI audio playback
                AudioManager::updateAudio(deltaTime);
                
                // Draw bottom screen UI
                UInGame::drawInGameUI();
            }

            // Stop cursor when leaving game
            CursorSystem::setEnabled(false);
            
            // Cleanup
            ScriptEngine::shutdown();
            UInGame::shutdown();
            PlayerController::shutdownPlayer();
            AudioManager::shutdownAudio();
            VoiceChat::shutdown();
            IconLoader::shutdown();
            CursorSystem::shutdown();
            AssetLoader::shutdown();
            ContentSystem::shutdown();
            ChatSystem::shutdown();
            DebugLogger::shutdown();
            // Re-init logger for next game session if enabled
            DebugLogger::init(Settings::get().debug.enableFileLogging);
            Settings::save("sdmc:/OpenBLOX/settings.sav");
            CitroRenderer::shutdown();
            
            // If leaving game, return to main menu
            if (leaveGame) {
                print_message("Returning to main menu...\n");
                leaveGame = false;
            }
        }
    } else {
        print_message("ERROR, ENGINE KILLED.\n\nIf you need help, look at:\n\nhttps://github.com/EGAMatsu/OpenBLOX/blob/main/README.md\n\nfor help, else join the discord:\nWf2bbU5z5J");
        while(1) {
            // Do nothing.
        }
    }
}

float test = 0;
void gameRenderLoop() {
    setupLighting();
    start3DFrame();
        test += 45*deltaTime;
        test = fmod(test, 360);
        /*scale_rotate_translate(1,1,1, -90,180,0, 0,0,0);
        scale_rotate_translate(1,1,1, 0,0,0, camera_x,camera_y,camera_z);*/
        scale_rotate_translate(1,1,1, camera_rx, camera_ry, 0, 0,0,0);
        scale_rotate_translate(1,1,1, 0, 0, 0, camera_x,camera_y,camera_z);
        renderWorld(dataModel);
        // Render cursor overlay on top of 3D world
        CursorSystem::render();
    endFrame();
}
