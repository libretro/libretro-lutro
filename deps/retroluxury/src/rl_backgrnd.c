#include <string.h>
#include <rl_backgrnd.h>
#include <rl_config.h>

#include <stdlib.h>

static uint16_t* pixels;
static int       width, height;
static uint16_t* fb;

int rl_backgrnd_create( int w, int h, int aspect )
{
  if ( aspect == RL_BACKGRND_EXACT )
  {
    pixels = (uint16_t*)malloc( ( ( w + RL_BACKGRND_MARGIN ) * h + RL_BACKGRND_MARGIN ) * sizeof( uint16_t ) );
    
    if ( pixels )
    {
      width  = w;
      height = h;
      fb     = (uint16_t*)pixels + RL_BACKGRND_MARGIN;
      
      return 0;
    }
  }
  
  return -1;
}

void rl_backgrnd_destroy( void )
{
  free( pixels );
}

void rl_backgrnd_clear( uint16_t color )
{
  uint16_t* pixel = fb;
  
  for ( int y = 0; y < height; y++ )
  {
    for ( int x = 0; x < width; x++ )
    {
      *pixel++ = color;
    }
    
    pixel += RL_BACKGRND_MARGIN;
  }
}

void rl_backgrnd_scroll( int dx, int dy )
{
  int       pitch  = width + RL_BACKGRND_MARGIN;
  uint16_t* dest   = fb;
  uint16_t* source = dest - dy * pitch - dx;
  int       count  = pitch * ( height - 1 ) + width;
  
  if ( dy > 0 )
  {
    source += dy * pitch;
    dest   += dy * pitch;
    count  -= dy * pitch;
  }
  
  if ( dy < 0 )
  {
    count += dy * pitch;
  }
  
  if ( dx > 0 )
  {
    source += dx;
    dest   += dx;
    count  -= dx;
  }
  
  if ( dx < 0 )
  {
    count += dx;
  }
  
  if ( count > 0 )
  {
    memmove( (void*)dest, (void*)source, count * sizeof( uint16_t ) );
  }
}

uint16_t* rl_backgrnd_fb( int* w, int* h )
{
  if ( w )
  {
    *w = width;
  }
  
  if ( h )
  {
    *h = height;
  }
  
  return fb;
}
