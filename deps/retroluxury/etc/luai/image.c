#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/*---------------------------------------------------------------------------*/
/* stb_image config and inclusion */

#define STBI_ASSERT( x )

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* stb_image_write config and inclusion */

#define STBIW_ASSERT( x )

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
/*---------------------------------------------------------------------------*/

#include <lua.h>
#include <lauxlib.h>

#if 0
static void dumpstack( lua_State* L )
{
  int top = lua_gettop( L );
  
  for ( int i = 1; i <= top; i++ )
  {
    printf( "%2d %3d ", i, i - top - 1 );
    
    lua_pushvalue( L, i );
    
    switch ( lua_type( L, -1 ) )
    {
    case LUA_TNIL:
      printf( "nil\n" );
      break;
    case LUA_TNUMBER:
      printf( "%e\n", lua_tonumber( L, -1 ) );
      break;
    case LUA_TBOOLEAN:
      printf( "%s\n", lua_toboolean( L, -1 ) ? "true" : "false" );
      break;
    case LUA_TSTRING:
      printf( "\"%s\"\n", lua_tostring( L, -1 ) );
      break;
    case LUA_TTABLE:
      printf( "table\n" );
      break;
    case LUA_TFUNCTION:
      printf( "function\n" );
      break;
    case LUA_TUSERDATA:
      printf( "userdata\n" );
      break;
    case LUA_TTHREAD:
      printf( "thread\n" );
      break;
    case LUA_TLIGHTUSERDATA:
      printf( "light userdata\n" );
      break;
    default:
      printf( "?\n" );
      break;
    }
  }
  
  lua_settop( L, top );
}
#endif

static const uint32_t s_crc32[] =
{
  0x00000000U, 0x77073096U, 0xee0e612cU, 0x990951baU,
  0x076dc419U, 0x706af48fU, 0xe963a535U, 0x9e6495a3U,
  0x0edb8832U, 0x79dcb8a4U, 0xe0d5e91eU, 0x97d2d988U,
  0x09b64c2bU, 0x7eb17cbdU, 0xe7b82d07U, 0x90bf1d91U,
  0x1db71064U, 0x6ab020f2U, 0xf3b97148U, 0x84be41deU,
  0x1adad47dU, 0x6ddde4ebU, 0xf4d4b551U, 0x83d385c7U,
  0x136c9856U, 0x646ba8c0U, 0xfd62f97aU, 0x8a65c9ecU,
  0x14015c4fU, 0x63066cd9U, 0xfa0f3d63U, 0x8d080df5U,
  0x3b6e20c8U, 0x4c69105eU, 0xd56041e4U, 0xa2677172U,
  0x3c03e4d1U, 0x4b04d447U, 0xd20d85fdU, 0xa50ab56bU,
  0x35b5a8faU, 0x42b2986cU, 0xdbbbc9d6U, 0xacbcf940U,
  0x32d86ce3U, 0x45df5c75U, 0xdcd60dcfU, 0xabd13d59U,
  0x26d930acU, 0x51de003aU, 0xc8d75180U, 0xbfd06116U,
  0x21b4f4b5U, 0x56b3c423U, 0xcfba9599U, 0xb8bda50fU,
  0x2802b89eU, 0x5f058808U, 0xc60cd9b2U, 0xb10be924U,
  0x2f6f7c87U, 0x58684c11U, 0xc1611dabU, 0xb6662d3dU,
  0x76dc4190U, 0x01db7106U, 0x98d220bcU, 0xefd5102aU,
  0x71b18589U, 0x06b6b51fU, 0x9fbfe4a5U, 0xe8b8d433U,
  0x7807c9a2U, 0x0f00f934U, 0x9609a88eU, 0xe10e9818U,
  0x7f6a0dbbU, 0x086d3d2dU, 0x91646c97U, 0xe6635c01U,
  0x6b6b51f4U, 0x1c6c6162U, 0x856530d8U, 0xf262004eU,
  0x6c0695edU, 0x1b01a57bU, 0x8208f4c1U, 0xf50fc457U,
  0x65b0d9c6U, 0x12b7e950U, 0x8bbeb8eaU, 0xfcb9887cU,
  0x62dd1ddfU, 0x15da2d49U, 0x8cd37cf3U, 0xfbd44c65U,
  0x4db26158U, 0x3ab551ceU, 0xa3bc0074U, 0xd4bb30e2U,
  0x4adfa541U, 0x3dd895d7U, 0xa4d1c46dU, 0xd3d6f4fbU,
  0x4369e96aU, 0x346ed9fcU, 0xad678846U, 0xda60b8d0U,
  0x44042d73U, 0x33031de5U, 0xaa0a4c5fU, 0xdd0d7cc9U,
  0x5005713cU, 0x270241aaU, 0xbe0b1010U, 0xc90c2086U,
  0x5768b525U, 0x206f85b3U, 0xb966d409U, 0xce61e49fU,
  0x5edef90eU, 0x29d9c998U, 0xb0d09822U, 0xc7d7a8b4U,
  0x59b33d17U, 0x2eb40d81U, 0xb7bd5c3bU, 0xc0ba6cadU,
  0xedb88320U, 0x9abfb3b6U, 0x03b6e20cU, 0x74b1d29aU,
  0xead54739U, 0x9dd277afU, 0x04db2615U, 0x73dc1683U,
  0xe3630b12U, 0x94643b84U, 0x0d6d6a3eU, 0x7a6a5aa8U,
  0xe40ecf0bU, 0x9309ff9dU, 0x0a00ae27U, 0x7d079eb1U,
  0xf00f9344U, 0x8708a3d2U, 0x1e01f268U, 0x6906c2feU,
  0xf762575dU, 0x806567cbU, 0x196c3671U, 0x6e6b06e7U,
  0xfed41b76U, 0x89d32be0U, 0x10da7a5aU, 0x67dd4accU,
  0xf9b9df6fU, 0x8ebeeff9U, 0x17b7be43U, 0x60b08ed5U,
  0xd6d6a3e8U, 0xa1d1937eU, 0x38d8c2c4U, 0x4fdff252U,
  0xd1bb67f1U, 0xa6bc5767U, 0x3fb506ddU, 0x48b2364bU,
  0xd80d2bdaU, 0xaf0a1b4cU, 0x36034af6U, 0x41047a60U,
  0xdf60efc3U, 0xa867df55U, 0x316e8eefU, 0x4669be79U,
  0xcb61b38cU, 0xbc66831aU, 0x256fd2a0U, 0x5268e236U,
  0xcc0c7795U, 0xbb0b4703U, 0x220216b9U, 0x5505262fU,
  0xc5ba3bbeU, 0xb2bd0b28U, 0x2bb45a92U, 0x5cb36a04U,
  0xc2d7ffa7U, 0xb5d0cf31U, 0x2cd99e8bU, 0x5bdeae1dU,
  0x9b64c2b0U, 0xec63f226U, 0x756aa39cU, 0x026d930aU,
  0x9c0906a9U, 0xeb0e363fU, 0x72076785U, 0x05005713U,
  0x95bf4a82U, 0xe2b87a14U, 0x7bb12baeU, 0x0cb61b38U,
  0x92d28e9bU, 0xe5d5be0dU, 0x7cdcefb7U, 0x0bdbdf21U,
  0x86d3d2d4U, 0xf1d4e242U, 0x68ddb3f8U, 0x1fda836eU,
  0x81be16cdU, 0xf6b9265bU, 0x6fb077e1U, 0x18b74777U,
  0x88085ae6U, 0xff0f6a70U, 0x66063bcaU, 0x11010b5cU,
  0x8f659effU, 0xf862ae69U, 0x616bffd3U, 0x166ccf45U,
  0xa00ae278U, 0xd70dd2eeU, 0x4e048354U, 0x3903b3c2U,
  0xa7672661U, 0xd06016f7U, 0x4969474dU, 0x3e6e77dbU,
  0xaed16a4aU, 0xd9d65adcU, 0x40df0b66U, 0x37d83bf0U,
  0xa9bcae53U, 0xdebb9ec5U, 0x47b2cf7fU, 0x30b5ffe9U,
  0xbdbdf21cU, 0xcabac28aU, 0x53b39330U, 0x24b4a3a6U,
  0xbad03605U, 0xcdd70693U, 0x54de5729U, 0x23d967bfU,
  0xb3667a2eU, 0xc4614ab8U, 0x5d681b02U, 0x2a6f2b94U,
  0xb40bbe37U, 0xc30c8ea1U, 0x5a05df1bU, 0x2d02ef8dU,
};

/*-----------------------------------------------------------------------------
Return a 32-bit crc for the given data block
-----------------------------------------------------------------------------*/

static uint32_t crc32( const void* data, int len )
{
  uint8_t* data_stream = ( uint8_t* )data;
  uint32_t crc = 0xedb88320U;

  if ( len <= 0 || data == 0 )
  {
    return 0;
  }

  while ( len )
  {
    uint8_t byte = *data_stream++;
    crc = ( crc >> 8 ) ^ s_crc32[ byte  ^ ( crc & 0xff ) ];
    len--;
  }

  return crc;
}

#define IMAGE_NAME "image"

typedef struct
{
  int width, height;
  uint32_t hash;
  uint32_t pixels[ 0 ];
}
image_t;

static image_t* check( lua_State* L, int index )
{
  return (image_t*)luaL_checkudata( L, index, IMAGE_NAME );
}

static int l_getwidth( lua_State* L )
{
  image_t* image = check( L, 1 );
  lua_pushinteger( L, image->width );
  return 1;
}

static int l_getheight( lua_State* L )
{
  image_t* image = check( L, 1 );
  lua_pushinteger( L, image->height );
  return 1;
}

static int l_getsize( lua_State* L )
{
  image_t* image = check( L, 1 );
  lua_pushinteger( L, image->width );
  lua_pushinteger( L, image->height );
  return 2;
}

static int l_clear( lua_State* L )
{
  image_t*  image = check( L, 1 );
  uint32_t  color = (uint32_t)luaL_optnumber( L, 2, 0xff000000 );
  uint32_t* pixel = image->pixels;
  int       i;
  
  for ( i = image->width * image->height; i > 0; i-- )
  {
    *pixel++ = color;
  }
  
  image->hash = 0;
  lua_settop( L, 1 );
  return 1;
}

static int l_putpixel( lua_State* L )
{
  image_t* image = check( L, 1 );
  int      x = (int)luaL_checkinteger( L, 2 );
  int      y = (int)luaL_checkinteger( L, 3 );
  uint32_t color = (uint32_t)luaL_checkinteger( L, 4 );
  
  if ( x < 0 || x >= image->width )
  {
    return luaL_error( L, "x coordinate outside [0, %d)", image->width );
  }
  
  if ( y < 0 || y >= image->height )
  {
    return luaL_error( L, "y coordinate outside [0, %d)", image->height );
  }
  
  image->pixels[ y * image->width + x ] = color;
  
  image->hash = 0;
  lua_settop( L, 1 );
  return 1;
}

static int l_getpixel( lua_State* L )
{
  image_t* image = check( L, 1 );
  int      x = (int)luaL_checkinteger( L, 2 );
  int      y = (int)luaL_checkinteger( L, 3 );
  
  if ( x < 0 || x >= image->width )
  {
    return luaL_error( L, "x coordinate outside [0, %d)", image->width );
  }
  
  if ( y < 0 || y >= image->height )
  {
    return luaL_error( L, "y coordinate outside [0, %d)", image->height );
  }
  
  lua_pushinteger( L, image->pixels[ y * image->width + x ] );
  return 1;
}

static int l_color2alpha( lua_State* L )
{
  image_t*  image = check( L, 1 );
  uint32_t  color = ( (uint32_t)luaL_optinteger( L, 2, 0 ) ) & 0x00ffffffU;
  uint32_t* pixel = image->pixels;
  int       i;
  
  for ( i = image->width * image->height; i > 0; i-- )
  {
    if ( ( *pixel & 0x00ffffffU ) == color )
    {
      *pixel = 0xff000000U;
    }
  }
  
  image->hash = 0;
  lua_settop( L, 1 );
  return 1;
}

#define MAX( a, b ) ( a > b ? a : b )

static int l_blit( lua_State* L )
{
  image_t* source = check( L, 1 );
  image_t* dest = check( L, 2 );
  int      x0 = (int)luaL_checkinteger( L, 3 );
  int      y0 = (int)luaL_checkinteger( L, 4 );
  int      left = (int)luaL_optinteger( L, 5, 0 );
  int      top = (int)luaL_optinteger( L, 6, 0 );
  int      right = (int)luaL_optinteger( L, 7, source->width - 1 );
  int      bottom = (int)luaL_optinteger( L, 8, source->height - 1 );
  int      yy = y0;
  int      x, y;
  
  for ( y = top; y <= bottom; y++ )
  {
    int xx = x0;
      
    for ( x = left; x <= right; x++ )
    {
      if ( xx >= 0 && xx < dest->width && yy >= 0 && yy < dest->height )
      {
        uint32_t sc = source->pixels[ y * source->width + x ];
        uint32_t dc = dest->pixels[ yy * dest->width + xx ];
        
        int sa = sc >> 24;
        int sr = sc & 255;
        int sg = ( sc >> 8 ) & 255;
        int sb = ( sc >> 16 ) & 255;
        
        int da = dc >> 24;
        int dr = dc & 255;
        int dg = ( dc >> 8 ) & 255;
        int db = ( dc >> 16 ) & 255;
        
        int r = ( sr * sa + dr * ( 255 - sa ) ) / 255;
        int g = ( sg * sa + dg * ( 255 - sa ) ) / 255;
        int b = ( sb * sa + db * ( 255 - sa ) ) / 255;
        
        dest->pixels[ yy * dest->width + xx ] = MAX( sa, da ) << 24 | b << 16 | g << 8 | r;
      }
      
      xx++;
    }
    
    yy++;
  }
  
  dest->hash = 0;
  lua_settop( L, 1 );
  return 1;
}

static int l_copy( lua_State* L )
{
  image_t* source = check( L, 1 );
  image_t* dest = check( L, 2 );
  int      x0 = (int)luaL_checkinteger( L, 3 );
  int      y0 = (int)luaL_checkinteger( L, 4 );
  int      left = (int)luaL_optinteger( L, 5, 0 );
  int      top = (int)luaL_optinteger( L, 6, 0 );
  int      right = (int)luaL_optinteger( L, 7, source->width - 1 );
  int      bottom = (int)luaL_optinteger( L, 8, source->height - 1 );
  int      yy = y0;
  int      x, y;
  
  for ( y = top; y <= bottom; y++ )
  {
    int xx = x0;
    
    for ( x = left; x <= right; x++ )
    {
      if ( xx >= 0 && xx < dest->width && yy >= 0 && yy < dest->height )
      {
        dest->pixels[ yy * dest->width + xx ] = source->pixels[ y * source->width + x ];
      }
      
      xx++;
    }
    
    yy++;
  }
  
  dest->hash = 0;
  lua_settop( L, 1 );
  return 1;
}

static int l_compare( lua_State* L )
{
  image_t*  source = check( L, 1 );
  image_t*  dest = check( L, 2 );
  uint32_t* sp = source->pixels;
  uint32_t* dp = dest->pixels;
  int       x, y;
  
  if ( source->hash == 0 )
  {
    source->hash = crc32( source->pixels, source->width * source->height * sizeof( uint32_t ) );
  }
  
  if ( dest->hash == 0 )
  {
    dest->hash = crc32( dest->pixels, dest->width * dest->height * sizeof( uint32_t ) );
  }
  
  if ( source->hash != dest->hash || source->width != dest->width || source->height != dest->height )
  {
    lua_pushboolean( L, 0 );
    return 1;
  }
  
  for ( y = 0; y < source->height; y++ )
  {
    for ( x = 0; x < source->width; x++ )
    {
      if ( *sp != *dp )
      {
        lua_pushboolean( L, 0 );
        return 1;
      }
      
      sp++, dp++;
    }
  }
  
  lua_pushboolean( L, 1 );
  return 1;
}

static int l_gethash( lua_State* L )
{
  image_t* image = check( L, 1 );
  
  if ( image->hash == 0 )
  {
    image->hash = crc32( image->pixels, image->width * image->height * sizeof( uint32_t ) );
  }
  
  lua_pushinteger( L, image->hash );
  return 1;
}

static int l_hasalpha( lua_State* L )
{
  image_t*  image = check( L, 1 );
  uint32_t* pixel = image->pixels;
  
  const uint32_t* end = image->pixels + image->width * image->height;

  while ( pixel < end )
  {
    if ( ( *pixel >> 24 ) != 255 )
    {
      lua_pushboolean( L, 1 );
      return 1;
    }
    
    pixel++;
  }
  
  lua_pushboolean( L, 0 );
  return 1;
}

static int l_invisible( lua_State* L )
{
  image_t*  image = check( L, 1 );
  uint32_t* pixel = image->pixels;
  
  const uint32_t* end = image->pixels + image->width * image->height;

  while ( pixel < end )
  {
    if ( ( *pixel >> 24 ) != 0 )
    {
      lua_pushboolean( L, 0 );
      return 1;
    }
    
    pixel++;
  }
  
  lua_pushboolean( L, 1 );
  return 1;
}

static image_t* push( lua_State* L, int width, int height );

static int l_diff( lua_State* L )
{
  image_t*  image = check( L, 1 );
  image_t*  other = check( L, 2 );
  image_t*  result;
  uint32_t* p1;
  uint32_t* p2;
  uint32_t* p3;
  int       i;
  
  if ( image->width != other->width || image->height != other->height )
  {
    return 0;
  }
  
  result = push( L, image->width, image->height );
  
  p1 = image->pixels;
  p2 = other->pixels;
  p3 = result->pixels;

  for ( i = image->width * image->height; i > 0; i-- )
  {
    if ( *p1 == *p2 )
    {
      *p3 = 0x00000000U;
    }
    else
    {
      *p3 = *p1;
    }
    
    p1++, p2++, p3++;
  }
  
  return 1;
}

static int l_band( lua_State* L )
{
  image_t*  image = check( L, 1 );
  image_t*  other = check( L, 2 );
  image_t*  result;
  uint32_t* p1;
  uint32_t* p2;
  uint32_t* p3;
  int       i;
  
  if ( image->width != other->width || image->height != other->height )
  {
    return 0;
  }
  
  result = push( L, image->width, image->height );
  
  p1 = image->pixels;
  p2 = other->pixels;
  p3 = result->pixels;

  for ( i = image->width * image->height; i > 0; i-- )
  {
    *p3++ = *p1++ & *p2++;
  }
  
  return 1;
}

static double min2( double a, double b )
{
  return a < b ? a : b;
}

static double min3( double a, double b, double c )
{
  return min2( min2( a, b ), c );
}

static double max2( double a, double b )
{
  return a > b ? a : b;
}

static double max3( double a, double b, double c )
{
  return max2( max2( a, b ), c );
}

static int rgb2hue( int r, int g, int b )
{
  double R = r / 255.0;
  double G = g / 255.0;
  double B = b / 255.0;
  
  double M = max3( R, G, B );
  double m = min3( R, G, B );
  double C = M - m;
  
  double H = 0.0;
  
  if ( C != 0.0 )
  {
    if ( M == R )
    {
      H = fmod( ( G - B ) / C, 6.0 );
    }
    else if ( M == G )
    {
      H = ( B - R ) / C + 2.0;
    }
    else if ( M == B )
    {
      H = ( R - G ) / C + 4.0;
    }
  }
  
  return (int)( H * 360.0 / 6.0 );
}

static int l_removecolor( lua_State* L )
{
  image_t*  image = check( L, 1 );
  int       hue = (int)luaL_checkinteger( L, 2 );
  int       dist = (int)luaL_checkinteger( L, 3 );
  image_t*  result = push( L, image->width, image->height );
  uint32_t* p1 = image->pixels;
  uint32_t* p2 = result->pixels;
  int       i;
  
  for ( i = image->width * image->height; i > 0; i-- )
  {
    uint32_t color = *p1++;
    
    int a = color >> 24;
    int r = color & 255;
    int g = ( color >> 8 ) & 255;
    int b = ( color >> 16 ) & 255;
    
    int h = rgb2hue( r, g, b );
    
    if ( abs( h - hue ) <= dist )
    {
      a = r = g = b = 0;
    }
    
    *p2++ = a << 24 | b << 16 | g << 8 | r;
  }
  
  return 1;
}

static int l_mirrorh( lua_State* L )
{
  image_t* image = check( L, 1 );
  image_t* mirror = push( L, image->width, image->height );
  int      x, y, z;
  
  for ( y = 0, z = image->height - 1; y < image->height; y++, z-- )
  {
    for ( x = 0; x < image->width; x++ )
    {
      mirror->pixels[ z * image->width + x ] = image->pixels[ y * image->width + x ];
    }
  }
  
  return 1;
}

static int l_sub( lua_State* L )
{
  image_t*  image = check( L, 1 );
  int       x0 = (int)luaL_checkinteger( L, 2 );
  int       y0 = (int)luaL_checkinteger( L, 3 );
  int       x1 = (int)luaL_checkinteger( L, 4 );
  int       y1 = (int)luaL_checkinteger( L, 5 );
  image_t*  sub;
  uint32_t* pixel;
  int       x, y;
  
  
  if ( x0 < 0 || x0 >= image->width )
  {
    return luaL_error( L, "x0 coordinate outsite of [0, %d)", image->width );
  }
  
  if ( y0 < 0 || y0 >= image->height )
  {
    return luaL_error( L, "y0 coordinate outsite of [0, %d)", image->height );
  }
  
  if ( x1 < 0 || x1 >= image->width )
  {
    return luaL_error( L, "x1 coordinate outsite of [0, %d)", image->width );
  }
  
  if ( y1 < 0 || y1 >= image->height )
  {
    return luaL_error( L, "y1 coordinate outsite of [0, %d)", image->height );
  }
  
  if ( x1 <= x0 )
  {
    return luaL_error( L, "x1 must be greater than x0" );
  }
  
  if ( y1 <= y0 )
  {
    return luaL_error( L, "y1 must be greater than y0" );
  }
  
  sub = push( L, x1 - x0 + 1, y1 - y0 + 1 );
  pixel = sub->pixels;
  
  for ( y = y0; y <= y1; y++ )
  {
    for ( x = x0; x <= x1; x++ )
    {
      *pixel++ = image->pixels[ y * image->width + x ];
    }
  }
  
  return 1;
}

static int l_save( lua_State* L )
{
  image_t*    image = check( L, 1 );
  const char* filename = luaL_checkstring( L, 2 );
  
  if ( stbi_write_png( filename, image->width, image->height, 4, (void*)image->pixels, image->width * sizeof( uint32_t ) ) )
  {
    lua_settop( L, 1 );
    return 1;
  }
  
  return luaL_error( L, "error saving %s", filename );
}

static image_t* push( lua_State* L, int width, int height )
{
  static const luaL_Reg methods[] =
  {
    { "getWidth",     l_getwidth },
    { "getHeight",    l_getheight },
    { "getSize",      l_getsize },
    { "clear",        l_clear },
    { "putPixel",     l_putpixel },
    { "getPixel",     l_getpixel },
    { "colorToAlpha", l_color2alpha },
    { "blit",         l_blit },
    { "copy",         l_copy },
    { "compare",      l_compare },
    { "getHash",      l_gethash },
    { "hasAlpha",     l_hasalpha },
    { "invisible",    l_invisible },
    { "diff",         l_diff },
    { "band",         l_band },
    { "removeColor",  l_removecolor },
    { "mirrorH",      l_mirrorh },
    { "sub",          l_sub },
    { "save",         l_save },
    { NULL, NULL }
  };
  
  image_t* image = (image_t*)lua_newuserdata( L, sizeof( image_t ) + width * height * sizeof( uint32_t ) );
  image->width = width;
  image->height = height;
  image->hash = 0;
  
  if ( luaL_newmetatable( L, IMAGE_NAME ) != 0 )
  {
    lua_pushvalue( L, -1 );
    lua_setfield( L, -2, "__index" );
    //luaL_register( L, NULL, methods );
    luaL_setfuncs( L, methods, 0 );
  }
  
  lua_setmetatable( L, -2 );
  
  return image;
}

static int l_create( lua_State* L )
{
  int      width = (int)luaL_checkinteger( L, 1 );
  int      height = (int)luaL_checkinteger( L, 2 );
  uint32_t color = (uint32_t)luaL_optinteger( L, 3, 0 );
  
  if ( width < 0 || width >= 65536 )
  {
    return luaL_error( L, "Width value must be in the [0, 65536) range" );
  }
  
  if ( height < 0 || height >= 65536 )
  {
    return luaL_error( L, "Height value must be in the [0, 65536) range" );
  }
  
  lua_pushcfunction( L, l_clear );
  
  push( L, width, height );
  lua_pushinteger( L, color );
  lua_call( L, 2, 1 );
  
  return 1;
}

static int l_load( lua_State* L )
{
  const char* filename = luaL_checkstring( L, 1 );
  int         width, height;
  uint32_t*   abgr32 = (uint32_t*)stbi_load( filename, &width, &height, NULL, STBI_rgb_alpha );
  
  if ( abgr32 )
  {
    image_t* image = push( L, width, height );
    memcpy( (void*)image->pixels, (void*)abgr32, width * height * sizeof( uint32_t ) );
    
    stbi_image_free( (void*)abgr32 );
    return 1;
  }
  
  return luaL_error( L, "error loading %s: %s", filename, stbi_failure_reason() );
}

static int l_color( lua_State* L )
{
  unsigned r = (unsigned)luaL_checkinteger( L, 1 );
  unsigned g = (unsigned)luaL_checkinteger( L, 2 );
  unsigned b = (unsigned)luaL_checkinteger( L, 3 );
  unsigned a = (unsigned)luaL_optinteger( L, 4, 255 );
  
  if ( r > 255 )
  {
    return luaL_error( L, "Red value must be in the [0, 255] range" );
  }
  
  if ( g > 255 )
  {
    return luaL_error( L, "Green value must be in the [0, 255] range" );
  }
  
  if ( b > 255 )
  {
    return luaL_error( L, "Blue value must be in the [0, 255] range" );
  }
  
  if ( a > 255 )
  {
    return luaL_error( L, "Alpha value must be in the [0, 255] range" );
  }
  
  lua_pushinteger( L, a << 24 | b << 16 | g << 8 | r );
  return 1;
}

static int l_split( lua_State* L )
{
  uint32_t color = (uint32_t)luaL_checkinteger( L, 1 );
  uint8_t  a = color >> 24;
  uint8_t  b = ( color >> 16 ) & 255;
  uint8_t  g = ( color >> 8 ) & 255;
  uint8_t  r = color & 255;
  
  lua_pushinteger( L, r );
  lua_pushinteger( L, g );
  lua_pushinteger( L, b );
  lua_pushinteger( L, a );
  return 4;
}

LUALIB_API int luaopen_image( lua_State* L )
{
  static const luaL_Reg statics[] =
  {
    { "create", l_create },
    { "load",   l_load },
    { "color",  l_color },
    { "split",  l_split },
    { NULL, NULL }
  };

  //luaL_register( L, "sdl", statics );
  luaL_newlib( L, statics );
  return 1;
}
