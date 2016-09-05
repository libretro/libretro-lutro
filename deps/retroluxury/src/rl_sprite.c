#include <rl_sprite.h>
#include <rl_config.h>

#include <stdlib.h>

typedef union item_t item_t;

union item_t
{
  struct
  {
    rl_sprite_t sprite;
    uint16_t*   bg;
  };
  
  item_t* next;
};

/* This is just to make it easier to reason about the sprites array */
typedef struct
{
  item_t* item;
}
spt_t;

static item_t  items[ RL_MAX_SPRITES ];
static item_t* free_list;

static spt_t sprites[ RL_MAX_SPRITES + 1 ];
static int   num_sprites, num_visible;

static uint16_t  saved_backgrnd[ RL_BG_SAVE_SIZE ];
static uint16_t* saved_ptr;

void rl_sprite_init( void )
{
  for ( int i = 0; i < RL_MAX_SPRITES - 1; i++ )
  {
    items[ i ].next = items + i + 1;
  }
  
  items[ RL_MAX_SPRITES - 1 ].next = NULL;
  free_list = items;
  
  num_sprites = num_visible = 0;
}

rl_sprite_t* rl_sprite_create( void )
{
  if ( num_sprites < RL_MAX_SPRITES )
  {
    rl_sprite_t* sprite = &free_list->sprite;
    sprites[ num_sprites++ ].item = free_list;
    free_list = free_list->next;
    
    sprite->layer = sprite->flags = 0;
    sprite->x = sprite->y = 0;
    sprite->image = NULL;
    
    return sprite;
  }
  
  return NULL;
}

static int compare( const void* e1, const void* e2 )
{
  const spt_t* s1 = (const spt_t*)e1;
  const spt_t* s2 = (const spt_t*)e2;
  
  int32_t c1 = s1->item->sprite.flags;
  int32_t c2 = s2->item->sprite.flags;
  
  c1 = c1 << 16 | s1->item->sprite.layer;
  c2 = c2 << 16 | s2->item->sprite.layer;
  
  return c1 - c2;
}

void rl_sprites_blit_nobg( void )
{
  spt_t* sptptr = sprites;
  const spt_t* endptr = sprites + num_sprites;
  
  if ( sptptr < endptr )
  {
    do
    {
      sptptr->item->sprite.flags &= ~RL_SPRITE_TEMP_INV;
      sptptr->item->sprite.flags |= sptptr->item->sprite.image == NULL;
      sptptr++;
    }
    while ( sptptr < endptr );
  }
  
  qsort( (void*)sprites, num_sprites, sizeof( spt_t ), compare );
  
  item_t guard = { { { { { 0 } } } } }; /* Ugh */
  guard.sprite.flags = RL_SPRITE_UNUSED;
  sprites[ num_sprites ].item = &guard;
  
  sptptr = sprites;
  
  /* Iterate over active and visible sprites and blit them */
  /* flags & 0x0007U == 0 */
  if ( sptptr->item->sprite.flags == 0 )
  {
    do
    {
      rl_image_blit_nobg( sptptr->item->sprite.image, sptptr->item->sprite.x, sptptr->item->sprite.y );
      sptptr++;
    }
    while ( sptptr->item->sprite.flags == 0 );
  }
  
  num_visible = sptptr - sprites;
  
  /* Jump over active but invisible sprites */
  /* flags & 0x0004U == 0x0000U */
  if ( ( sptptr->item->sprite.flags & RL_SPRITE_UNUSED ) == 0 )
  {
    do
    {
      sptptr++;
    }
    while ( ( sptptr->item->sprite.flags & RL_SPRITE_UNUSED ) == 0 );
  }
  
  int new_num_sprites = sptptr - sprites;
  
  /* Iterate over unused sprites and free them */
  /* flags & 0x0004U == 0x0004U */
  endptr = sprites + num_sprites;
  
  if ( sptptr < endptr )
  {
    do
    {
      sptptr->item->next = free_list;
      free_list = sptptr->item;
      sptptr++;
    }
    while ( sptptr < endptr );
  }
  
  num_sprites = new_num_sprites;
}

void rl_sprites_blit( void )
{
  spt_t* sptptr = sprites;
  const spt_t* endptr = sprites + num_sprites;
  
  if ( sptptr < endptr )
  {
    do
    {
      sptptr->item->sprite.flags &= ~RL_SPRITE_TEMP_INV;
      sptptr->item->sprite.flags |= sptptr->item->sprite.image == NULL;
      sptptr++;
    }
    while ( sptptr < endptr );
  }
  
  qsort( (void*)sprites, num_sprites, sizeof( spt_t ), compare );
  
  item_t guard = { { { { { 0 } } } } }; /* Ugh */
  guard.sprite.flags = RL_SPRITE_UNUSED;
  sprites[ num_sprites ].item = &guard;
  
  sptptr = sprites;
  saved_ptr = saved_backgrnd;
  
  /* Iterate over active and visible sprites and blit them */
  /* flags & 0x0007U == 0 */
  if ( sptptr->item->sprite.flags == 0 )
  {
    do
    {
      sptptr->item->bg = saved_ptr;
      saved_ptr = rl_image_blit( sptptr->item->sprite.image, sptptr->item->sprite.x, sptptr->item->sprite.y, saved_ptr );
      sptptr++;
    }
    while ( sptptr->item->sprite.flags == 0 );
  }
  
  num_visible = sptptr - sprites;
  
  /* Jump over active but invisible sprites */
  /* flags & 0x0004U == 0x0000U */
  if ( ( sptptr->item->sprite.flags & RL_SPRITE_UNUSED ) == 0 )
  {
    do
    {
      sptptr++;
    }
    while ( ( sptptr->item->sprite.flags & RL_SPRITE_UNUSED ) == 0 );
  }
  
  int new_num_sprites = sptptr - sprites;
  
  /* Iterate over unused sprites and free them */
  /* flags & 0x0004U == 0x0004U */
  endptr = sprites + num_sprites;
  
  if ( sptptr < endptr )
  {
    do
    {
      sptptr->item->next = free_list;
      free_list = sptptr->item;
      sptptr++;
    }
    while ( sptptr < endptr );
  }
  
  num_sprites = new_num_sprites;
}

void rl_sprites_unblit( void )
{
  spt_t* sptptr = sprites + num_visible - 1;
  
  /* Unblit the sprites in reverse order */
  if ( sptptr >= sprites )
  {
    do
    {
      rl_image_unblit( sptptr->item->sprite.image, sptptr->item->sprite.x, sptptr->item->sprite.y, sptptr->item->bg );
      sptptr--;
    }
    while ( sptptr >= sprites );
  }
}
