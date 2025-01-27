
require("graphics/base_transform")

local rotation = 0
local xflip = 1
local yflip = 1
local time = 0

return {
	intervalTime = 12,

	load = function()
		load_transform_tiles()
	end,

	draw = function()
		draw_transform_tiles{ scalex = xflip, scaley = yflip }
	end,

	update = function(dt)
		time = time + dt

		local lutx = { 1,  1, -1, -1 }
		local luty = { 1, -1,  1, -1 }

		local ix = math.min(math.floor(time / 3) + 1, 4)
		local iy = math.min(math.floor(time / 3) + 1, 4)
		xflip = lutx[ix]
		yflip = luty[iy]
	end
}
