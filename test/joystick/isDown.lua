return {
	draw = function()
		lutro.graphics.print("1: " .. tostring(lutro.joystick.isDown(1, 1)), 30, 100)
		lutro.graphics.print("2: " .. tostring(lutro.joystick.isDown(1, 2)), 100, 100)
		lutro.graphics.print("3: " .. tostring(lutro.joystick.isDown(1, 3)), 200, 100)
		lutro.graphics.print("4: " .. tostring(lutro.joystick.isDown(1, 4)), 30, 200)
		lutro.graphics.print("5: " .. tostring(lutro.joystick.isDown(1, 5)), 100, 200)
		lutro.graphics.print("6: " .. tostring(lutro.joystick.isDown(1, 6)), 200, 200)
		lutro.graphics.print("7: " .. tostring(lutro.joystick.isDown(1, 7)), 30, 300)
		lutro.graphics.print("8: " .. tostring(lutro.joystick.isDown(1, 8)), 100, 300)
		lutro.graphics.print("9: " .. tostring(lutro.joystick.isDown(1, 9)), 200, 300)
	end
}
