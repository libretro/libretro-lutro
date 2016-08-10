#include <rl_rand.h>

static uint64_t seed;

void rl_srand( uint64_t s )
{
  seed = s;
}

uint32_t rl_rand( void )
{
  /*
  http://en.wikipedia.org/wiki/Linear_congruential_generator
  Newlib parameters
  */
  
  seed = 6364136223846793005ULL * seed + 1;
  return seed >> 32;
}

int rl_random( int min, int max )
{
  /* fixed point math */
  uint64_t dividend = max - min + 1;
  return (int)( ( ( dividend * rl_rand() ) >> 32 ) + min );
}
