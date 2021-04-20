#ifndef LUTRO_STB_IMAGE
#define LUTRO_STB_IMAGE

#include <stdint.h>

int lutro_stb_image_load(const char* filename, uint32_t** data, unsigned int* width, unsigned int* height);

#endif
