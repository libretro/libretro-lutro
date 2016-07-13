#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#include <lua.h>
#include <lauxlib.h>

char __cdecl *realpath( const char *__restrict__ name, char *__restrict__ resolved );

#define SEPARATOR '/'
#define OTHER_SEP '\\'

static int l_realpath( lua_State* L )
{
  const char* path = luaL_checkstring( L, 1 );
  char        buffer[ _MAX_PATH ];
  char*       resolved = realpath( path, buffer );
  
  if ( resolved != NULL )
  {
    while ( *resolved != 0 )
    {
      if ( *resolved == OTHER_SEP )
      {
        *resolved = SEPARATOR;
      }
      
      resolved++;
    }
    
    lua_pushstring( L, buffer );
    return 1;
  }
  
  lua_pushstring( L, strerror( errno ) );
  return lua_error( L );
}

static int is_separator( char k )
{
  return k == SEPARATOR || k == OTHER_SEP;
}

/*static int l_sanitize( lua_State* L )
{
  size_t length;
  const char* path = luaL_checklstring( L, 1, &length );
  
  if ( length < _MAX_PATH - 1 )
  {
    char source[ _MAX_PATH + 1 ];
    source[ 0 ] = SEPARATOR;
    memcpy( source + 1, path, length );
    
    char* dest = source;
    
    for ( size_t i = 1; i <= length; )
    {
      if ( source[ i ] == '\\' && source[ i + 1 ] == ' ' )
      {
        *dest++ = ' ';
      }
      else if ( IsSeparator( source[ i ] ) )
      {
        if ( source[ i + 1 ] == '.' )
        {
          if ( IsSeparator( source[ i + 2 ] ) )
          {
            *dest++ = SEPARATOR;
            i += 3;
          }
          else if ( source[ i + 2 ] == '.' && IsSeparator( source[ i + 3 ] ) )
          {
            while ( *dest != SEPARATOR )
            {
              dest--;
            }
            
            if ( dest == source + 1 )
            {
              return luaL_error( L, "Invalid path" );
            }
            
            dest++;
            i += 4;
          }
        }
        else if ( !IsSeparator( dest[ -1 ] ) )
        {
          *dest++ = SEPARATOR;
          i++;
        }
      }
      else
      {
        *dest++ = source[ i++ ];
      }
    }
    
    lua_pushlstring( L, source + 1, dest - source - 1 );
    return 1;
  }
  
  return luaL_error( L, "Path too long" );
}
*/

static int l_split( lua_State* L )
{
  size_t      length;
  const char* path = luaL_checklstring( L, 1, &length );
  const char* ext = path + length;
  
  while ( ext >= path && *ext != '.' && !is_separator( *ext ) )
  {
    ext--;
  }
  
  const char* name = ext;
  
  if ( *ext != '.' )
  {
    ext = NULL;
  }
  
  while ( name >= path && !is_separator( *name ) )
  {
    name--;
  }

  if ( is_separator( *name ) )
  {
    name++;
  }

  if ( name - path - 1 > 0 )
  {
    lua_pushlstring( L, path, name - path - 1 );
  }
  else
  {
    lua_pushliteral( L, "" );
  }
  
  if ( ext != NULL )
  {
    lua_pushlstring( L, name, ext - name );
    lua_pushstring( L, ext );
  }
  else
  {
    lua_pushstring( L, name );
    lua_pushliteral( L, "" );
  }
  
  return 3;
}

static int l_scandir( lua_State* L )
{
  const char* name = luaL_checkstring( L, 1 );
  DIR* dir = opendir( name );
  
  if ( dir )
  {
    struct dirent* entry;
    int ndx = 1;
    
    lua_createtable( L, 0, 0 );
    
    while ( ( entry = readdir( dir ) ) != NULL )
    {
      lua_pushfstring( L, "%s%c%s", name, SEPARATOR, entry->d_name );
      lua_rawseti( L, -2, ndx++ );
    }
    
    closedir( dir );
    return 1;
  }
  
  lua_pushstring( L, strerror( errno ) );
  return lua_error( L );
}

static void pushtime( lua_State* L, time_t* time )
{
  struct tm* tm = gmtime( time );
  char buf[ 256 ];
  sprintf( buf, "%04d-%02d-%02dT%02d:%02d:%02dZ", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec );
  lua_pushstring( L, buf );
}

static int l_stat( lua_State* L )
{
  static const struct { unsigned flag; const char* name; } modes[] =
  {
    //{ S_IFSOCK, "sock" },
    //{ S_IFLNK,  "link" },
    { S_IFREG,  "file" },
    { S_IFBLK,  "block" },
    { S_IFDIR,  "dir" },
    { S_IFCHR,  "char" },
    { S_IFIFO,  "fifo" },
  };
  
  const char* name = luaL_checkstring( L, 1 );
  struct stat buf;
  int i;
  
  if ( stat( name, &buf ) == 0 )
  {
    lua_createtable( L, 0, 0 );
    
    lua_pushinteger( L, buf.st_size );
    lua_setfield( L, -2, "size" );
    
    pushtime( L, &buf.st_atime );
    lua_setfield( L, -2, "atime" );
    
    pushtime( L, &buf.st_mtime );
    lua_setfield( L, -2, "mtime" );
    
    pushtime( L, &buf.st_ctime );
    lua_setfield( L, -2, "ctime" );
    
    for ( i = 0; i < sizeof( modes ) / sizeof( modes[ 0 ] ); i++ )
    {
      lua_pushboolean( L, ( buf.st_mode & S_IFMT ) == modes[ i ].flag );
      lua_setfield( L, -2, modes[ i ].name );
    }
    
    return 1;
  }
  
  lua_pushstring( L, strerror( errno ) );
  return lua_error( L );
}

LUAMOD_API int luaopen_path( lua_State* L )
{
  static const luaL_Reg statics[] =
  {
    { "realpath", l_realpath },
    //{ "sanitize", l_sanitize },
    { "split",    l_split },
    { "scandir",  l_scandir },
    { "stat",     l_stat },
    { NULL, NULL }
  };

  luaL_newlib( L, statics );
  
  lua_pushfstring( L, "%c", SEPARATOR );
  lua_setfield( L, -2, "separator" );
  
  return 1;
}
