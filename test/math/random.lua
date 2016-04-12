lutro.math.setRandomSeed(3)
lutro.math.setRandomSeed(3, 10)

return {
	draw = function()
    local rand1 = lutro.math.random()
		lutro.graphics.print("lutro.math.random       " .. rand1, 30, 100)

    local rand2 = lutro.math.random(10)
		lutro.graphics.print("lutro.math.random 0-10  " .. rand2, 30, 150)

    local rand3 = lutro.math.random(10, 20)
		lutro.graphics.print("lutro.math.random 10-20 " .. rand3, 30, 200)
	end
}
