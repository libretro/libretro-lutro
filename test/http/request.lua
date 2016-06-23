local http = require('socket.http')
local result = 'Result'
local err = 'Request not made yet...'

return {
	load = function()
		result, err = http.request("http://wrong.host/")
	end,
	draw = function()
		lutro.graphics.print(tostring(err), 30, 100)
	end
}
