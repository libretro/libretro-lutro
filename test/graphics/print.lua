return {
	load = function()
		font = lutro.graphics.newImageFont("graphics/font.png", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+/")
		lutro.graphics.setFont(font)
	end,

	draw = function()
		lutro.graphics.print("Tests switch every 3 seconds.", 30, 100)

		if lutro.mouse.isDown(1, 2) then
			lutro.graphics.print("The left or right mouse button is Down.", 30, 200)
		end
	end
}
