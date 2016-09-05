#include <rl_imgdata.h>

/*---------------------------------------------------------------------------*/
/* stb_image config and inclusion */

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT( x )
#define STBI_NO_STDIO
#include "external/stb_image.h"
/*---------------------------------------------------------------------------*/

int rl_imgdata_create( rl_imgdata_t* imgdata, const void* data, size_t size )
{
  int width, height;
  uint32_t* abgr = (uint32_t*)stbi_load_from_memory( data, size, &width, &height, NULL, STBI_rgb_alpha );
  
  if ( abgr )
  {
    imgdata->width = width;
    imgdata->height = height;
    imgdata->pitch = width;
    imgdata->abgr = abgr;
    imgdata->parent = NULL;
    
    return 0;
  }
  
  return -1;
}

int rl_imgdata_sub( rl_imgdata_t* imgdata, const rl_imgdata_t* parent, int x0, int y0, int width, int height )
{
  if ( x0 < 0 )
  {
    width += x0;
    x0 = 0;
  }
  
  if ( ( x0 + width ) > parent->width )
  {
    width = parent->width - x0;
  }
  
  if ( y0 < 0 )
  {
    height += y0;
    y0 = 0;
  }
  
  if ( ( y0 + height ) > parent->height )
  {
    height = parent->height - y0;
  }
  
  if ( width > 0 && height > 0 )
  {
    imgdata->width = width;
    imgdata->height = height;
    imgdata->pitch = parent->pitch;
    imgdata->abgr = parent->abgr + y0 * parent->pitch + x0;
    imgdata->parent = parent;
    
    return 0;
  }
  
  return -1;
}

void rl_imgdata_destroy( const rl_imgdata_t* imgdata )
{
  if ( !imgdata->parent )
  {
    free( (void*)imgdata->abgr );
  }
}

uint32_t rl_imgdata_get_pixel( const rl_imgdata_t* imgdata, int x, int y )
{
  if ( x >= 0 && x < imgdata->width && y >= 0 && y < imgdata->height )
  {
    return imgdata->abgr[ y * imgdata->pitch + x ];
  }
  
  return 0;
}

static inline uint16_t rgb24_to_rgb16( int r, int g, int b )
{
  r >>= 3;
  g >>= 2;
  b >>= 3;
  
  return ( r << 11 ) | ( g << 5 ) | b;
}

static inline uint16_t get_pixel( int* alpha, const rl_imgdata_t* imgdata, int x, int y, int check_transp, uint16_t transparent )
{
  uint32_t abgr32 = rl_imgdata_get_pixel( imgdata, x, y );
  int      a = abgr32 >> 24;
  int      b = ( abgr32 >> 16 ) & 255;
  int      g = ( abgr32 >> 8 ) & 255;
  int      r = abgr32 & 255;
  uint16_t rgb16 = rgb24_to_rgb16( r, g, b );
  
  if ( check_transp && rgb16 == transparent )
  {
    a = rgb16 = 0;
  }
  
  if ( a >= 0 && a <= 31 )
  {
    a = 0; /* transparent */
  }
  else if ( a >= 32 && a <= 95 )
  {
    a = 1; /* 25% */
  }
  else if ( a >= 96 && a <= 159 )
  {
    a = 2; /* 50% */
  }
  else if ( a >= 160 && a <= 223 )
  {
    a = 3; /* 75% */
  }
  else
  {
    a = 4; /* opaque */
  }
  
  *alpha = a;
  return rgb16;
}

static size_t rle_row( uint16_t* rle, int* bgcount, const rl_imgdata_t* imgdata, int y, int check_transp, uint16_t transparent )
{
  int width = imgdata->width;
  int num_cols = ( width + ( RL_BACKGRND_MARGIN - 1 ) ) / RL_BACKGRND_MARGIN;
  int dryrun = !rle;
  
  if ( dryrun )
  {
    rle = (uint16_t*)&dryrun;
  }
  
  uint16_t* start = rle;
  uint16_t* cols = rle;
  rle += num_cols;
  
  for ( int xx = 0; xx < width; xx += RL_BACKGRND_MARGIN )
  {
    if ( !dryrun )
    {
      *cols++ = rle - start;
    }
    
    int       x = xx;
    uint16_t* runs = rle++;
    
    if ( !dryrun )
    {
      *runs = 0;
    }
    
    while ( x < ( xx + RL_BACKGRND_MARGIN ) && x < width )
    {
      int a;
      get_pixel( &a, imgdata, x, y, check_transp, transparent );
      
      int count = 1;
      int xsave = x++;
      
      while ( x < ( xx + RL_BACKGRND_MARGIN ) && x < width )
      {
        int aa;
        get_pixel( &aa, imgdata, x, y, check_transp, transparent );
        
        if ( aa != a )
        {
          break;
        }
        
        count++, x++;
      }
      
      if ( !dryrun )
      {
        *rle = ( a << 13 ) | count;
        ( *runs )++;
      }
      
      rle++;
      
      if ( a )
      {
        for ( int i = 0; i < count; i++ )
        {
          uint16_t rgb16 = get_pixel( &a, imgdata, xsave + i, y, check_transp, transparent );
          ( *bgcount )++;
          
          if ( !dryrun )
          {
            *rle = rgb16;
          }
          
          rle++;
        }
      }
    }
  }
  
  return ( rle - start ) * 2;
}

const void* rl_imgdata_encode( size_t* size, const rl_imgdata_t* imgdata, int check_transp, uint16_t transparent )
{
  int    width = imgdata->width;
  int    height = imgdata->height;
  int    bgcount = 0;
  size_t total = 0;
  
  for ( int y = 0; y < height; y++ )
  {
    total += rle_row( NULL, &bgcount, imgdata, y, check_transp, transparent );
  }
  
  void* rle = malloc(
    sizeof( uint16_t ) +          /* width */
    sizeof( uint16_t ) +          /* height */
    sizeof( uint32_t ) +          /* bgcount */
    height * sizeof( uint32_t ) + /* row pointers */
    total                         /* rle data */
  );
  
  if ( rle )
  {
    union
    {
      void*     restrict v;
      uint8_t*  restrict u8;
      uint16_t* restrict u16;
      uint32_t* restrict u32;
    }
    ptr;
    
    ptr.v = rle;
    
    *ptr.u16++ = width;
    *ptr.u16++ = height;
    *ptr.u32++ = bgcount;
    
    total = height * sizeof( uint32_t );
    uint8_t* rledata = ptr.u8 + total;
    
    for ( int y = 0; y < height; y++ )
    {
      *ptr.u32++    = total;
      size_t count  = rle_row( (uint16_t*)rledata, &bgcount, imgdata, y, check_transp, transparent );
      total        += count;
      rledata      += count;
    }
    
    *size = rledata - (uint8_t*)rle;
    return rle;
  }
  
  return NULL;
}
