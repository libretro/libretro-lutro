
local tiles = {}
local quads = {}

function load_transform_tiles()
    local tile_sizes = { 32, 64, 128, 256 }
    for _, tsz in ipairs(tile_sizes) do
        if tiles[tsz] == nil then
            tiles[tsz] = lutro.graphics.newImage(("graphics/grid-transform-%dpx.png"):format(tsz))
            quads[tsz] = lutro.graphics.newQuad(0, 0, tsz, tsz, tsz, tsz)
        end
    end
end

function draw_transform_tiles(params)
    lutro.graphics.setColor(255, 255, 255)

    local scalex = params.scalex or 1
    local scaley = params.scaley or 1

    -- draw multiple fixed scales of the quad onscreen at once.
    -- avoid use of animation since it complicates automated verification 
    -- display multiple screens of tiled tests instead.
    -- Include scales that converge toward zero to ensure safety of maths (DivZero, etc)
    -- use the first tile of the 2nd column to display scale=0 to ensure it's handled correctly (DivZero)

    local xpos = 10
    local ypos = 40

    local xscales = {
        1.00, 1.00,  1.00,  1.00, 1.00,
        0.00, 0.70,  0.50,  0.30, 0.001
    }

    local yscales = {
        1.00, 0.70,  0.50,  0.30, 0.001,
        0.00, 1.00,  1.00,  1.00, 1.00,
    }

    local tsz = 128
    local adv_y = tsz + 4

    for i = 1, 10 do
        if i == 6 then
            xpos = 10
            ypos = ypos + adv_y + 15
        end
        local orig = tsz / 2
        lutro.graphics.draw(
            tiles[tsz], quads[tsz],
            xpos + orig, ypos + orig,
            rotation,
            xscales[i] * scalex, yscales[i] * scaley,
            orig, orig
        )
        lutro.graphics.print(("%.1f x %.1f"):format(xscales[i] * scalex, yscales[i] * scaley), xpos + 32, ypos - 15)
        xpos = xpos + tsz + 5
    end
end