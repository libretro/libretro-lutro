local homeDir = lutro.filesystem.getUserDirectory()

return {
	draw = function()
		lutro.graphics.print(homeDir, 30, 100)
	end
}
