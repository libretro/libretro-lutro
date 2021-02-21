function lutro.window.minimizeTest()
	lutro.window.minimize()
end

function lutro.window.maximizeTest()
	lutro.window.maximize()
end

function lutro.window.getTitleTest()
	local title = lutro.window.getTitle()
	unit.assertIsString(title)
	unit.assertEquals(title, "Lutro")
end

function lutro.window.setTitleTest()
	lutro.window.setTitle("Hello World!")
end

function lutro.window.setPositionTest()
	lutro.window.setPosition(1, 1)
	lutro.window.setPosition(100, 200, 2)
end

function lutro.window.getPositionTest()
	local x, y, display = lutro.window.getPosition()
	unit.assertIsNumber(x)
	unit.assertIsNumber(y)
	unit.assertIsNumber(display)
	unit.assertEquals(x, 0)
	unit.assertEquals(y, 0)
	unit.assertEquals(display, 1)
end

function lutro.window.requestAttentionTest()
	lutro.window.requestAttention()
	lutro.window.requestAttention("Test")
end

function lutro.window.getDisplayNameTest()
	local display = lutro.window.getDisplayName()
	unit.assertIsString(display)
	unit.assertEquals(display, "libretro")

	display = lutro.window.getDisplayName(3)
	unit.assertIsString(display)
	unit.assertEquals(display, "libretro")
end

function lutro.window.setDisplaySleepEnabledTest()
	lutro.window.setDisplaySleepEnabled(true)
end

return {
    lutro.window.minimizeTest,
    lutro.window.maximizeTest,
    lutro.window.getTitleTest,
    lutro.window.setTitleTest,
    lutro.window.setPositionTest,
    lutro.window.getPositionTest,
    lutro.window.requestAttentionTest,
    lutro.window.getDisplayNameTest,
    lutro.window.setDisplaySleepEnabledTest
}