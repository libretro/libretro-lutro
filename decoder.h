#ifndef DECODER_H
#define DECODER_H

#include <vorbis/vorbisfile.h>

typedef struct
{
	OggVorbis_File vf;
	vorbis_info *info;
} OggData;

bool decoder_initOgg(OggData *data, char *filename);
bool decoder_seek(OggData *data, uint32_t pos);
bool decoder_sampleTell(OggData *data, uint32_t *pos);
bool decoder_decodeOgg(OggData *data, float *buffer, float volume, bool loop);

#endif // DECODER_H
