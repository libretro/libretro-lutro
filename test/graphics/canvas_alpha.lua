-- A semi-transparent fill over a solid part of a canvas must leave the canvas
-- solid. The canvas is drawn over magenta, so any magenta showing through the
-- grey strip is a hole in the canvas.
local steps = 16
local stepWidth = 34
local canvas

return {
	load = function()
		local width = steps * stepWidth
		canvas = lutro.graphics.newCanvas(width, 120)
		lutro.graphics.setCanvas(canvas)
		lutro.graphics.setColor(200, 200, 200, 255)
		lutro.graphics.rectangle("fill", 0, 0, width, 120)

		-- Black at alpha 0, 17, ..., 255: darkens evenly from grey to black.
		for step = 0, steps - 1 do
			lutro.graphics.setColor(0, 0, 0, step * 17)
			lutro.graphics.rectangle("fill", step * stepWidth, 20, stepWidth, 80)
		end
		lutro.graphics.setCanvas()
	end,

	draw = function()
		lutro.graphics.setColor(255, 0, 255, 255)
		lutro.graphics.rectangle("fill", 28, 80, steps * stepWidth + 40, 160)
		lutro.graphics.setColor(255, 255, 255, 255)
		lutro.graphics.draw(canvas, 48, 100)
	end
}
