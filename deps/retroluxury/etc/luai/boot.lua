local path = require 'path'

return function( args )
  -- Check for script file name
  if not args[ 0 ] then
    error( 'Lua script missing' )
  end
  
  -- Resolve full path to the Lua script
  args[ 0 ] = path.realpath( args[ 0 ] )
  
  -- Add the Lua script folder to the Lua path
  --local dir = path.split( args[ 0 ] )
  --package.path = package.path .. ';' .. dir .. '/?.lua'
  
  -- Run the script
  local chunk = loadfile( args[ 0 ] )
  local main = chunk()
  return main( args )
end
