function lutro.keyboard.setBackgroundColorTest()
    red = 115
    green = 27
    blue = 135
    alpha = 50
    color = { red, green, blue, alpha }
    lutro.graphics.setBackgroundColor(color)
    lutro.graphics.setBackgroundColor(red, green, blue, alpha)
end

function lutro.keyboard.getBackgroundColorTest()
	r, g, b, a = lutro.graphics.getBackgroundColor()
	unit.assertEquals(r, 115)
	unit.assertEquals(g, 27)
	unit.assertEquals(b, 135)
	unit.assertEquals(a, 50)
end

return {
    lutro.keyboard.setBackgroundColorTest,
    lutro.keyboard.getBackgroundColorTest
}