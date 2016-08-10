return function()
  return {
    bytes = {},
    add8 = function( self, x )
      self.bytes[ #self.bytes + 1 ] = string.char( x & 255 )
    end,
    add16 = function( self, x )
      self.bytes[ #self.bytes + 1 ] = string.char( ( x >> 8 ) & 255, x & 255 )
    end,
    add32 = function( self, x )
      self.bytes[ #self.bytes + 1 ] = string.char( ( x >> 24 ) & 255, ( x >> 16 ) & 255, ( x >> 8 ) & 255, x & 255 )
    end,
    addstring = function( self, str )
      self.bytes[ #self.bytes + 1 ] = str
    end,
    addwriter = function( self, rle )
      self.bytes[ #self.bytes + 1 ] = table.concat( rle.bytes )
    end,
    align = function( self, x )
      local size = self:getsize()
      local new_size = ( size + x - 1 ) & -x
      
      while size < new_size do
        self:add8( 0 )
        size = size + 1
      end
    end,
    getbytes = function( self )
      self.bytes = { table.concat( self.bytes ) }
      return self.bytes[ 1 ]
    end,
    getsize = function( self )
      return #self:getbytes()
    end,
    save = function( self, name )
      local file = assert( io.open( name, 'wb' ) )
      file:write( self:getbytes() )
      file:close()
    end
  }
end
