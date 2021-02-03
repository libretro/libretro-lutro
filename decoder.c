#include <libretro.h>
#include <audio/audio_mix.h>
#include <audio/conversion/float_to_s16.h>

#include "decoder.h"
#include "audio.h"

#define CHANNELS 2
float floatData[AUDIO_FRAMES * CHANNELS];

//
bool decoder_initOgg(OggData *data, char *filename)
{
	data->info = NULL;
	if (ov_fopen(filename, &data->vf) < 0)
	{
		printf("Failed to open vorbis file: %s", filename);
		return false;
	}

	printf("vorbis info:\n");
	printf("\tnum streams: %d\n",  ov_streams(&data->vf));

	data->info = (vorbis_info*)ov_info(&data->vf, 0);
	if (!data->info)
	{
		printf("couldn't get info for file");
		return false;
	}

	printf("\tnum channels: %d\n", data->info->channels);
	printf("\tsample rate: %d\n", data->info->rate);

	if (data->info->channels != 1 && data->info->channels != 2)
	{
		printf("unsupported number of channels\n");
		return false;
	}
	
	if (data->info->rate != 44100)
	{
		printf("unsupported sample rate\n");
		return false;
	}

	printf("vorbis init success\n");
	return true;
}

//
bool decoder_seek(OggData *data, uint32_t pos)
{
	return ov_pcm_seek(&data->vf, pos) == 0;
}

//
bool decoder_sampleTell(OggData *data, uint32_t *pos)
{
	ogg_int64_t ret = ov_pcm_tell(&data->vf);
	if (ret >= 0)
	{
		*pos = ret;
		return true;
	}
	
	return false;
}

//
bool decoder_decodeOgg(OggData *data, float *buffer, float volume, bool loop)
{
	//printf("decoder_decodeOgg\n");

	size_t frames = AUDIO_FRAMES;
	size_t rendered = 0;
	bool finished = false;

	float *dst = floatData;

	while (frames)
	{
		float **pcm;
		int bitstream;
		//printf("pcmoffs: %d\n", data->vf.pcm_offset);
		long ret = ov_read_float(&data->vf, &pcm, frames, &bitstream);

		if (ret < 0)
		{
			printf("Vorbis decoding failed with: %d\n", ret);
			return true;
		}

		if (ret == 0) // EOF
		{
			if (loop)
			{
				if (ov_time_seek(&data->vf, 0.0) == 0)
					continue;               
				else
					finished = true;
			}
			else
				finished = true;

			break;
		}

		if (data->info->channels == 2)
		{
			for (long i = 0; i < ret; i++)
			{
				dst[i * CHANNELS + 0] = pcm[0][i];
				dst[i * CHANNELS + 1] = pcm[1][i];
			}
		}
		else
		{
			for (long i = 0; i < ret; i++)
			{
				dst[i * CHANNELS + 0] = pcm[0][i];
				dst[i * CHANNELS + 1] = pcm[0][i];
			}
		}

		dst += ret * CHANNELS;
		frames -= ret;
		rendered += ret;
	}
	
	audio_mix_volume(buffer, floatData, volume, rendered * CHANNELS);
	return finished;
}
