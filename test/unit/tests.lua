-- Add all available include paths.
package.path = package.path .. './test/?.lua;./test/unit/?.lua;./test/unit/luaunit/?.lua;./luaunit/?.lua'

-- Dependencies
unit = require 'luaunit'

function UTF8Test()
	local utf8 = require("utf8")
	local actual = utf8.len("Hello World!")
	unit.assertEquals(actual, 12)
end

function lutro.filesystem.setRequirePathTest()
	local paths = lutro.filesystem.getRequirePath()
	paths = paths .. ';/lutro.filesystem.setRequirePathTest/?.lua'
	lutro.filesystem.setRequirePath(paths)

	local paths = lutro.filesystem.getRequirePath()
	local index = string.find(paths, 'setRequirePathTest')
	unit.assertTrue(index > 1)
end

function lutro.filesystem.getRequirePathTest()
	local paths = lutro.filesystem.getRequirePath()
	unit.assertIsString(paths)
	unit.assertEquals(paths, package.path)
end

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

function lutro.math.setRandomSeedTest()
	lutro.math.setRandomSeed(3)
	lutro.math.setRandomSeed(3, 10)
end

function lutro.math.randomTest()
	for i = 1, 1000, 1 do
		actual = lutro.math.random(10, 20)
		unit.assertTrue(actual >= 10 and actual <= 20)
	end

	for i = 1, 1000, 1 do
		actual = lutro.math.random(10)
		unit.assertTrue(actual >= 0 and actual <= 10)
	end

	for i = 1, 1000, 1 do
		actual = lutro.math.random()
		unit.assertTrue(actual >= 0 and actual <= 1)
	end
end

function lutro.filesystem.getUserDirectoryTest()
	local homeDir = lutro.filesystem.getUserDirectory()
	-- @todo Find out how to make os.getenv('HOME') work on Windows?
	local luaHomeDir = os.getenv("HOME")
	unit.assertEquals(homeDir, luaHomeDir)
end

function lutro.getVersionTest()
	local major, minor, revision, codename = lutro.getVersion()
	unit.assertIsNumber(major)
	unit.assertIsNumber(minor)
	unit.assertIsNumber(revision)
	unit.assertEquals(codename, 'Lutro')
end

function lutro.system.getProcessorCountTest()
	local count = lutro.system.getProcessorCount()
	unit.assertIsNumber(count)
	unit.assertEquals(count, 1)
end

function lutro.system.getClipboardTextTest()
	local clipboard = lutro.system.getClipboardText()
	unit.assertIsString(clipboard)
	unit.assertEquals(clipboard, "")
end

function lutro.system.setClipboardTextTest()
	local clipboard = "Hello, World!"
	lutro.system.setClipboardText(clipboard)

	clipboard = "Goodbye, World!"
	clipboard = lutro.system.getClipboardText()
	unit.assertEquals(clipboard, "Hello, World!")
end

-- Runs all the defined tests.
function runTests()
	lutro.keyboard.getKeyFromScancodeTest()
	lutro.keyboard.getScancodeFromKeyTest()
	lutro.filesystem.getUserDirectoryTest()
	lutro.getVersionTest()
	lutro.math.randomTest()
	lutro.math.setRandomSeedTest()
	UTF8Test()
	lutro.filesystem.getRequirePathTest()
	lutro.filesystem.setRequirePathTest()

	lutro.system.getProcessorCountTest()
	lutro.system.getClipboardTextTest()
	lutro.system.setClipboardTextTest()
end

-- Return a load and draw function for running the unit
-- tests in the Lutro testing suite.
return {
	load = function()
		runTests()
	end,

	draw = function()
		lutro.graphics.print('Unit tests have been run.', 30, 100)
	end
}
