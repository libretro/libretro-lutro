#ifndef RL_XML_H
#define RL_XML_H

#include <rl_userdata.h>

#include <stddef.h>

typedef struct rl_xmlhandlers_t rl_xmlhandlers_t;

struct rl_xmlhandlers_t
{
  rl_userdata_t ud;
  
  int ( *start_document )( rl_xmlhandlers_t* handlers );
  int ( *end_document )( rl_xmlhandlers_t* handlers );
  int ( *start_element )( rl_xmlhandlers_t* handlers, const char* name, size_t length );
  int ( *end_element )( rl_xmlhandlers_t* handlers, const char* name, size_t length );
  int ( *attribute )( rl_xmlhandlers_t* handlers, const char* key, size_t key_length, const char* value, size_t value_length );
  int ( *characters )( rl_xmlhandlers_t* handlers, const char* text, size_t length );
};

int rl_xml_parse( const char* xml, size_t length, rl_xmlhandlers_t* handlers );

#endif /* RL_XML_H */
