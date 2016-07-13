#include <rl_base64.h>

size_t rl_base64_decode( const void* buffer, size_t length, void* output )
{
  static const uint8_t values[] =
  {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,
     0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,
     0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  };
  
  size_t rest = length % 4;
  length -= rest;
  uint8_t* bin = (uint8_t*)output;
  const uint8_t* digits = (uint8_t*)buffer;
  const uint8_t* end = digits + length;
  uint8_t d0, d1, d2, d3;
  uint8_t b0, b1, b2;
  
  if ( digits < end )
  {
    do
    {
      d0 = values[ digits[ 0 ] ];
      d1 = values[ digits[ 1 ] ];
      d2 = values[ digits[ 2 ] ];
      d3 = values[ digits[ 3 ] ];
      
      b0 = d0 << 2 | d1 >> 4;
      b1 = ( d1 & 0x0f ) << 4 | d2 >> 2;
      b2 = ( d2 & 0x03 ) << 6 | d3;
      
      bin[ 0 ] = b0;
      bin[ 1 ] = b1;
      bin[ 2 ] = b2;
      
      digits += 4;
      bin += 3;
    }
    while ( digits < end );
  }
  
  if ( rest == 0 )
  {
    /* No remaining characters, check for padding. */
    if ( length )
    {
      bin -= ( digits[ -1 ] == '=' ) + ( digits[ -2 ] == '=' );
    }
  }
  else if ( rest == 3 )
  {
    /* Three characters remaining without padding. */
    d0 = values[ digits[ 0 ] ];
    d1 = values[ digits[ 1 ] ];
    d2 = values[ digits[ 2 ] ];
    
    b0 = d0 << 2 | d1 >> 4;
    b1 = ( d1 & 0x0f ) << 4 | d2 >> 2;
    
    bin[ 0 ] = b0;
    bin[ 1 ] = b1;
    bin += 2;
  }
  else if ( rest == 2 )
  {
    /* Two characters remaining without padding. */
    d0 = values[ digits[ 0 ] ];
    d1 = values[ digits[ 1 ] ];
  
    b0 = d0 << 2 | d1 >> 4;
    
    *bin++ = b0;
  }
  
  /* rest == 1 --> One character remaining without padding (invalid). */
  
  return bin - (uint8_t*)output;
}
