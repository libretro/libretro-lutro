#ifndef RL_SPRITE_H
#define RL_SPRITE_H

#include <rl_image.h>
#include <rl_backgrnd.h>
#include <rl_userdata.h>

#include <stdint.h>

/*
A sprite is basically an image with spatial position, a layer index for depth
sorting, and a visibility flag. Sprites also automatically save the background
pixels when their images are blit so it can be restored before the beginning
of the next frame.
*/

#define RL_SPRITE_TEMP_INV  0x0001U
#define RL_SPRITE_INVISIBLE 0x0002U
#define RL_SPRITE_UNUSED    0x0004U

typedef struct
{
  rl_userdata_t ud;
  
  uint16_t layer;
  uint16_t flags;
  int      x;
  int      y;
  
  rl_image_t* image; /* the sprite's image */
}
rl_sprite_t;

void rl_sprite_init( void );

rl_sprite_t* rl_sprite_create( void );
#define      rl_sprite_destroy( sprite ) do { ( sprite )->flags |= RL_SPRITE_UNUSED; } while ( 0 )

void rl_sprites_blit_nobg( void );

void rl_sprites_blit( void );
void rl_sprites_unblit( void );

#endif /* RL_SPRITE_H */
