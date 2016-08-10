#ifndef RL_RAND_H
#define RL_RAND_H

#include <stdint.h>

void     rl_srand( uint64_t seed );
uint32_t rl_rand( void );
int      rl_random( int min, int max );

#endif /* RL_RAND_H */
