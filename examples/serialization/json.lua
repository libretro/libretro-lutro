-- json parsing
function json_decode(msg)
	local js = {}
	for k,v in string.gmatch(msg,'"(%w+)":"(%w+)",') do
	   js[k] = v;
	end
	for k,v in string.gmatch(msg,'"(%w+)":"(%w+)"}') do
	   js[k] = v;
	end
	return js
 end
 
 function json_encode(map)
	local msg = "{"
	for k,v in pairs(map) do
	   msg = msg..'"'..k..'":"'..v..'",'
	end
	return string.sub(msg,0,-2)..'}'
 end