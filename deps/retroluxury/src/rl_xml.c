#include <rl_xml.h>

#include <string.h>
#include <ctype.h>

static inline void skip_to_tag( const char** xml )
{
  const char* aux = *xml;
  
  while ( *aux != '<' && *aux )
  {
    aux++;
  }
  
  *xml = aux;
}

static inline void skip_name( const char** xml )
{
  const char* aux = *xml;
  
  while ( !isspace( *aux ) && *aux != '>' && *aux != '=' && *aux )
  {
    aux++;
  }
  
  *xml = aux;
}

static inline void skip_spaces( const char** xml )
{
  const char* aux = *xml;
  
  while ( isspace( *aux ) )
  {
    aux++;
  }
  
  *xml = aux;
}

#define CHECK( stmt ) do { int res = stmt; if ( res ) return res; } while ( 0 )

static int parse( const char* xml, const char* end, rl_xmlhandlers_t* handlers )
{
  CHECK( handlers->start_document( handlers ) );
  
  do
  {
    const char* text = xml;
    skip_to_tag( &xml );
    
    if ( xml != text )
    {
      CHECK( handlers->characters( handlers, text, xml - text ) );
    }
    
    if ( *xml != '<' )
    {
      goto out;
    }
    
    if ( xml[ 1 ] == '/' )
    {
      const char* name = xml += 2;
      skip_name( &xml );
      CHECK( handlers->end_element( handlers, name, xml - name ) );
      skip_spaces( &xml );
      
      if ( *xml != '>' )
      {
        goto out;
      }
      
      xml++;
    }
    else if ( xml[ 1 ] == '?' )
    {
      xml = strstr( xml + 2, "?>" );
      
      if ( !xml )
      {
        goto out; 
      }
      
      xml += 2;
    }
    else
    {
      const char* name = ++xml;
      skip_name( &xml );
      size_t name_length = xml - name;
      CHECK( handlers->start_element( handlers, name, name_length ) );
      
      skip_spaces( &xml );
      
      while ( *xml != '>' && *xml != '/' )
      {
        const char* key = xml;
        skip_name( &xml );
        size_t key_length = xml - key;
        
        skip_spaces( &xml );
        
        if ( *xml != '=' )
        {
          goto out;
        }
        
        xml++;
        skip_spaces( &xml );
        char quote = *xml++;
        const char* value = xml;
        
        xml = strchr( xml, quote );
        
        if ( !xml )
        {
          goto out;
        }
        
        CHECK( handlers->attribute( handlers, key, key_length, value, xml - value ) );
        
        xml++;
        skip_spaces( &xml );
      }
      
      if ( *xml == '/' )
      {
        CHECK( handlers->end_element( handlers, name, name_length ) );
        xml++;
        skip_spaces( &xml );
        
        if ( *xml != '>' )
        {
          goto out;
        }
      }
      
      xml++;
    }
  }
  while ( *xml );
  
  out:
  return handlers->end_document( handlers );
}

int rl_xml_parse( const char* xml, size_t length, rl_xmlhandlers_t* handlers )
{
  return parse( xml, xml + length, handlers );
}
