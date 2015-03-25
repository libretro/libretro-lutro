local x = 0
local xspeed = 40

return {
	draw = function()
		lutro.graphics.setColor(255, 0, 0)
		lutro.graphics.rectangle("fill", x, 100, 100, 80)
	end,

	update = function(dt)
		x = x + xspeed * dt
		if x + 100 > 640 then
			xspeed = -40
		elseif x < 0 then
			xspeed = 40
		end
	end
}
