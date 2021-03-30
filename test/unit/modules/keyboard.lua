function lutro.keyboard.getScancodeFromKeyTest()
	local scancode = lutro.keyboard.getScancodeFromKey('f')
	unit.assertIsNumber(scancode)
	unit.assertEquals(scancode, 102)
end

function lutro.keyboard.getKeyFromScancodeTest()
	local key = lutro.keyboard.getKeyFromScancode(102)
	unit.assertIsString(key)
	unit.assertEquals(key, 'f')
end

return {
    lutro.keyboard.getScancodeFromKeyTest,
    lutro.keyboard.getKeyFromScancodeTest
}