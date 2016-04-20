return {
	load = function()
		count = lutro.joystick.getJoystickCount()
	end,
	draw = function()
		lutro.graphics.print(tostring(count), 30, 100)
	end
}
