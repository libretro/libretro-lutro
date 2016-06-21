local utf8 = require("utf8")

return {
	load = function()
		font = lutro.graphics.newImageFont("graphics/font.png", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+/")
		lutro.graphics.setFont(font)
	end,

	draw = function()
		-- Output the version information.
		local major, minor, revision, codename = lutro.getVersion()
		local str = string.format("Version %d.%d.%d - %s", major, minor, revision, codename)
		lutro.graphics.print(str, 10, 50)

		-- Display test instructions.
		text = "Tests switch every 3 seconds. UTF-8 length: "
		len = utf8.len(text)
		lutro.graphics.print(text .. len, 30, 100)

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
