return {
	load = function()
		font = lutro.graphics.newImageFont("graphics/font.png", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+/")
		lutro.graphics.setFont(font)
	end,

	draw = function()
		local major, minor, revision, codename = lutro.getVersion()
		local str = string.format("Version %d.%d.%d - %s", major, minor, revision, codename)
		lutro.graphics.print(str, 20, 20)
		lutro.graphics.print("Tests switch every 3 seconds.", 30, 100)

		if lutro.mouse.isDown(1, 2) then
			local x, y = lutro.mouse.getPosition()
			lutro.graphics.print("Mouse " .. lutro.mouse.getX() .. "-" .. lutro.mouse.getY(), x, y)
		end
		if lutro.keyboard.isDown('space', 'a') then
			lutro.graphics.print("Space or A key is down!", 200, 200)
		end
	end
}
