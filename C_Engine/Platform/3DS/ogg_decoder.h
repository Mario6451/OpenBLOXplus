// Lightweight wrapper header for OGG decoding helper
#pragma once
#include <string>

// Decode an OGG file at 'path'. On success returns true and sets channels, sample_rate,
// pcm_out (allocated by stb_vorbis via malloc) and samples (per-channel sample count).
// Caller must free(pcm_out) when finished.
bool decode_ogg_to_pcm(const std::string &path, int *channels, int *sample_rate, short **pcm_out, int *samples_out);
