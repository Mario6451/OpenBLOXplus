#include "ogg_decoder.h"
#include <cstdlib>
#include <string>

extern "C" {
	// stb_vorbis implementation is compiled as a C source in the build (ThirdParty/stb_vorbis.c)
	int stb_vorbis_decode_filename(const char *filename, int *channels, int *sample_rate, short **output);
}

bool decode_ogg_to_pcm(const std::string &path, int *channels, int *sample_rate, short **pcm_out, int *samples_out) {
	if (!pcm_out || !samples_out) return false;
	*pcm_out = nullptr;
	*samples_out = 0;
	int ch = 0, sr = 0;
	short *pcm = nullptr;
	int samples = stb_vorbis_decode_filename(path.c_str(), &ch, &sr, &pcm);
	if (samples > 0 && pcm) {
		*channels = ch;
		*sample_rate = sr;
		*pcm_out = pcm;
		*samples_out = samples;
		return true;
	}
	return false;
}
