#ifndef RL_IMAGE_H
#define RL_IMAGE_H

#include <rl_userdata.h>
#include <rl_imgdata.h>

#include <stdint.h>
#include <stdlib.h>

/*
An image with RLE-encoded pixels and per-pixel alpha of 0, 25, 50, 75 and 100%.
Images save the background in the bg pointer when blit, and restore the
background with that information when unblit.
*/

typedef struct
{
  rl_userdata_t ud;
  
  int width;     /* the image width */
  int height;    /* the image height */
  int used;      /* number of overwritten pixels on the background */
  
  const void* rle;
  
  union
  {
    const uint8_t*  data; /* stored row offsets and rle data */
    const uint32_t* rows; /* offsets to rle data for each row */
  };
}
rl_image_t;

void rl_image_init( void );
void rl_image_translate( int x, int y );

/* Creates an image from a rl_imgdata_t. */
int     rl_image_create( rl_image_t* image, const rl_imgdata_t* imgdata, int check_transp, uint16_t transparent );
/* Destroys an image. */
#define rl_image_destroy( image ) do { free( (void*)( image )->rle ); } while ( 0 )

/* Blits an image to the given background. */
void      rl_image_blit_nobg( const rl_image_t* image, int x, int y );
/* Blits an image to the given background, saving overwritten pixels in bg. */
uint16_t* rl_image_blit( const rl_image_t* image, int x, int y, uint16_t* bg );
/* Erases an image from the given background, restoring overwritten pixels from bg. */
void      rl_image_unblit( const rl_image_t* image, int x, int y, const uint16_t* bg );

#endif /* RL_IMAGE_H */
