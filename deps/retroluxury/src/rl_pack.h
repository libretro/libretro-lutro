#ifndef RL_PACK_H
#define RL_PACK_H

#include <stdint.h>

/* Flag that the archive/entry has already been converted. */
#define RL_ENDIAN_CONVERTED 0x80000000UL

typedef struct
{
  uint32_t name_hash;
  uint32_t name_offset;
  uint32_t data_offset;
  uint32_t data_size;
  
  uint32_t runtime_flags;
}
rl_entry_t;

typedef struct
{
  uint32_t num_entries;
  uint32_t runtime_flags;
  
  rl_entry_t entries[ 0 ];
}
rl_header_t;

rl_entry_t* rl_find_entry( void* data, const char* name );

#endif /* RL_PACK_H */
