#ifndef RL_BASE64_H
#define RL_BASE64_H

#include <stdint.h>
#include <stddef.h>

size_t  rl_base64_decode( const void* buffer, size_t length, void* output );
#define rl_base64_decode_inplace( buffer, length ) do { return rl_base64_decode( ( buffer ), ( length ), ( buffer ) ); } while ( 0 )

#endif /* RL_BASE64_H */
