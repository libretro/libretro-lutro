#include <stdint.h>
#include "lutro_stb_image.h"
#include "streams/file_stream.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_NO_THREAD_LOCALS
#include "stb/stb_image.h"

/**
 * Load the given image into the data buffer.
 *
 * @return 1 on success, 0 on error.
 */
int lutro_stb_image_load(const char* filename, uint32_t** data, unsigned int* width, unsigned int* height) {
   void* buf;
   ssize_t len;
   int x, y, channels_in_file;
   int channels = 4;

   // Load the file data.
   if (filestream_read_file(filename, &buf, &len) <= 0) {
      return 0;
   }

   // Load the data as an image.
   stbi_uc* output = stbi_load_from_memory((stbi_uc const*)buf, (int)len, &x, &y, &channels_in_file, channels);
   *width = x;
   *height = y;
   free(buf);

   // Ensure the image loaded successfully.
   if (output == NULL) {
      return 0;
   }

   // Flip the bits to have them in the format Lutro uses.
   {
      int rValue = 0;
      int bValue = 2;
      for (int xIndex = 0; xIndex < x; xIndex++) {
         for (int yIndex = 0; yIndex < y; yIndex++) {
            int index = xIndex + x * yIndex;
            stbi_uc tmp;
            tmp = output[index * channels + rValue];
            output[index * channels + rValue] = output[index * channels + bValue];
            output[index * channels + bValue] = tmp;
         }
      }
   }

   // Return the output as a success.
   *data = (uint32_t*)output;

   return 1;
}
