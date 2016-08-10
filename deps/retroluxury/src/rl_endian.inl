static inline uint16_t be16( uint16_t x )
{
  static const union
  {
    uint16_t u16;
    uint8_t u8[ 2 ];
  }
  u = { 1 };
  
  return u.u8[ 0 ] ? x >> 8 | x << 8 : x;
}

static inline uint32_t be32( uint32_t x )
{
  static const union
  {
    uint16_t u16;
    uint8_t u8[ 2 ];
  }
  u = { 1 };
  
  if ( u.u8[ 0 ] )
  {
    return be16( x ) << 16 | be16( x >> 16 );
  }
  else
  {
    return x;
  }
}

static inline uint16_t le16( uint16_t x )
{
  static const union
  {
    uint16_t u16;
    uint8_t u8[ 2 ];
  }
  u = { 1 };
  
  return u.u8[ 0 ] ? x : x >> 8 | x << 8;
}

static inline uint32_t le32( uint32_t x )
{
  static const union
  {
    uint16_t u16;
    uint8_t u8[ 2 ];
  }
  u = { 1 };
  
  if ( u.u8[ 0 ] )
  {
    return x;
  }
  else
  {
    return le16( x ) << 16 | le16( x >> 16 );
  }
}

static inline int isle( void )
{
  static const union
  {
    uint16_t u16;
    uint8_t u8[ 2 ];
  }
  u = { 1 };
  
  return u.u8[ 0 ];
}

static inline int isbe( void )
{
  static const union
  {
    uint16_t u16;
    uint8_t u8[ 2 ];
  }
  u = { 1 };
  
  return !u.u8[ 0 ];
}
