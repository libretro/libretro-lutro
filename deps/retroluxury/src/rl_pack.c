#include <rl_pack.h>

#include <string.h>

#include <rl_endian.inl>
#include <rl_hash.inl>

rl_entry_t* rl_find_entry( void* data, const char* name )
{
  rl_header_t* header = (rl_header_t*)data;
  
  if ( !( header->runtime_flags & RL_ENDIAN_CONVERTED ) )
  {
    if ( isle() )
    {
      header->num_entries = be32( header->num_entries );
      
      for ( uint32_t i = 0; i < header->num_entries; i++ )
      {
        header->entries[ i ].name_hash     = be32( header->entries[ i ].name_hash );
        header->entries[ i ].name_offset   = be32( header->entries[ i ].name_offset );
        header->entries[ i ].data_offset   = be32( header->entries[ i ].data_offset );
        header->entries[ i ].data_size     = be32( header->entries[ i ].data_size );
        header->entries[ i ].runtime_flags = 0;
      }
    }
    
    header->runtime_flags = RL_ENDIAN_CONVERTED;
  }
  
  uint32_t name_hash = djb2( name );
  
  for ( uint32_t i = 0; i < header->num_entries - 1; i++ )
  {
    uint32_t ndx = ( name_hash + i ) % header->num_entries;
    rl_entry_t* entry = header->entries + ndx;
    
    if ( entry->name_hash == name_hash )
    {
      const char* entry_name = (char*)data + entry->name_offset;
      
      if ( !strcmp( name, entry_name ) )
      {
        return entry;
      }
    }
  }
  
  return NULL;
}
