#ifndef RL_BACKGRND_H
#define RL_BACKGRND_H

#include <stdint.h>

#define RL_BACKGRND_EXACT   0
//#define RL_BACKGRND_STRETCH 1
//#define RL_BACKGRND_EXPAND  2

#define RL_COLOR( r, g, b ) ( ( ( r ) * 32 / 256 ) << 11 | ( ( g ) * 64 / 256 ) << 5 | ( ( b ) * 32 / 256 ) )
#define RL_COLOR_2( rgb ) RL_COLOR( ( rgb >> 16 ) & 255, ( rgb >> 8 ) & 255, rgb & 255 )

int  rl_backgrnd_create( int width, int height, int aspect );
void rl_backgrnd_destroy( void );

void      rl_backgrnd_clear( uint16_t color );
void      rl_backgrnd_scroll( int dx, int dy );
uint16_t* rl_backgrnd_fb( int* width, int* height );

#endif /* RL_BACKGRND_H */
