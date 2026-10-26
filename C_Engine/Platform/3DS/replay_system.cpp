#include "replay_system.h"
#include <cstdio>
#include <cstring>
#include <dirent.h>

namespace Replay {
    static const char* REPLAY_DIR = "sdmc:/OpenBLOX/replays/";
    static std::vector<ReplayFrame> frames;
    static bool recording = false;
    static bool playing = false;
    static int playbackIndex = 0;
    static u64 startTime = 0;

    void init() {
        std::string cmd = std::string("mkdir -p ") + REPLAY_DIR;
        system(cmd.c_str());
    }

    void startRecording() {
        frames.clear();
        recording = true;
        startTime = osGetTime();
    }

    bool stopRecording(const std::string& name) {
        recording = false;
        if (frames.empty()) return false;

        char path[256];
        snprintf(path, sizeof(path), "%s%s.replay", REPLAY_DIR, name.c_str());
        FILE* f = fopen(path, "wb");
        if (!f) return false;

        u32 count = (u32)frames.size();
        fwrite(&count, sizeof(u32), 1, f);
        fwrite(&startTime, sizeof(u64), 1, f);
        fwrite(frames.data(), sizeof(ReplayFrame), count, f);
        fclose(f);
        return true;
    }

    bool isRecording() { return recording; }

    bool loadReplay(const std::string& name) {
        char path[256];
        snprintf(path, sizeof(path), "%s%s.replay", REPLAY_DIR, name.c_str());
        FILE* f = fopen(path, "rb");
        if (!f) return false;

        u32 count = 0;
        fread(&count, sizeof(u32), 1, f);
        fread(&startTime, sizeof(u64), 1, f);

        frames.resize(count);
        fread(frames.data(), sizeof(ReplayFrame), count, f);
        fclose(f);
        playbackIndex = 0;
        return true;
    }

    void startPlayback() {
        if (!frames.empty()) {
            playing = true;
            playbackIndex = 0;
        }
    }

    void stopPlayback() {
        playing = false;
        playbackIndex = 0;
    }

    bool isPlaying() { return playing; }

    ReplayFrame getCurrentFrame() {
        if (playbackIndex < frames.size()) {
            return frames[playbackIndex];
        }
        return {0, 0, 0.0f, 0.0f};
    }

    float getDuration() {
        if (frames.empty()) return 0.0f;
        return (float)(frames.back().timestamp - frames.front().timestamp) / 1000.0f;
    }

    std::vector<std::string> listReplays() {
        std::vector<std::string> result;
        DIR* dir = opendir(REPLAY_DIR);
        if (!dir) return result;

        struct dirent* ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".replay")) {
                std::string name = ent->d_name;
                name = name.substr(0, name.size() - 7); // Remove .replay
                result.push_back(name);
            }
        }
        closedir(dir);
        return result;
    }

    bool deleteReplay(const std::string& name) {
        char path[256];
        snprintf(path, sizeof(path), "%s%s.replay", REPLAY_DIR, name.c_str());
        return remove(path) == 0;
    }

    void shutdown() {
        if (recording) stopRecording("auto_save");
        frames.clear();
    }
}