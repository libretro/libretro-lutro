local t, rotation, scale_x, scale_y, org_x, org_y, src_x, src_w = 0, 0, 0, 0, 0, 0, 0, 0
local draw_line = false
local line_segments = {}
local img = nil

return {
	load = function()
		img = lutro.graphics.newImage("graphics/font.png")
		line_segments[1] = {x=104, y=108} -- 100 + centred on quad
	end,

	draw = function()
		lutro.graphics.setColor(255, 255, 255)

		-- track scaling-offset interaction using a line
		if draw_line then
			lutro.graphics.print("Image should follow the line:", 40, 80)
			line_segments[#line_segments + 1] = {x=104 - scale_x * org_x, y=108 - scale_y * org_y}
			for idx, coord in ipairs(line_segments) do
				if idx > 1 then
					local prevcoord = line_segments[idx - 1]
					lutro.graphics.line(prevcoord.x, prevcoord.y, coord.x, coord.y)
				end
			end
		end

		lutro.graphics.draw(img,
			lutro.graphics.newQuad( src_x, 0, src_w, 16, img:getWidth(), img:getHeight()),
			100, 100,
			rotation,
			scale_x,
			scale_y,
			org_x,
			org_y
		)
	end,

	update = function(dt)
		-------------------------------------------
		-- Set parameters based on current time. --
		-------------------------------------------
		t = t + dt

		-- rotation not supported, so we leave it as zero.
		rotation = 0

		scale_x = 1 + math.sin(t * 3) * 4
		scale_y = math.cos(t * 2) * 2

		-- test zero scaling (should be blank)
		if t < 0.2 then
			scale_x = 0
			scale_y = 0
		end
		
		-- image offset while scaled
		if t > 1.5 then
			-- set xscale, yscale to something arbitrary
			-- then ensure origin is offset correctly by scale
			scale_x = 2.5 - math.pow(t - 1.5, 9)
			scale_y = 1.1 + math.pow(t - 1.5, 7)
			org_x = -(t - 1.5) * 100
			org_y = org_x + math.sin((t - 1.5) * 8) * 20
			-- (origin should follow the line being drawn)
			draw_line = true
		end

		-- src position
		src_x = 9 * math.floor(t + 1)
		src_w = 8
	end
}
