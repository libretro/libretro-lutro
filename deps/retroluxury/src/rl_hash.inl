static inline uint32_t djb2( const char* str )
{
  const unsigned char* aux = (const unsigned char*)str;
  uint32_t hash = 5381;

  while ( *aux )
  {
    hash = ( hash << 5 ) + hash + *aux++;
  }

  return hash;
}

static inline uint32_t djb2_length( const char* str, size_t length )
{
  const unsigned char* aux = (const unsigned char*)str;
  uint32_t hash = 5381;

  while ( length-- )
  {
    hash = ( hash << 5 ) + hash + *aux++;
  }

  return hash;
}
