return {
	load = function()
	end,

	draw = function()
		-- Show mouse information.
		if lutro.mouse.isDown(1, 2) then
			local x, y = lutro.mouse.getPosition()
			lutro.graphics.print("Mouse " .. lutro.mouse.getX() .. "-" .. lutro.mouse.getY(), x, y)
		end
		if lutro.keyboard.isDown('space', 'a') then
			lutro.graphics.print("Space or A key is down!", 200, 200)
		end
	end
}
