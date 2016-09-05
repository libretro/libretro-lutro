return function( str )
  local hash = 5381
  
  for i = 1, #str do
    hash = hash * 33 + str:byte( i )
    hash = hash & 0xffffffff
  end
  
  return hash
end
