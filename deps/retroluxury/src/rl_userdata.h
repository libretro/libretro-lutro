#ifndef RL_USERDATA_H
#define RL_USERDATA_H

#include <rl_config.h>

typedef union
{
  void* p;
  int   i;
}
rl_uditem_t;

typedef rl_uditem_t rl_userdata_t[ RL_USERDATA_COUNT ];

#endif /* RL_USERDATA_H */
