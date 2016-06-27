-- Add all available include paths.
package.path = package.path .. './test/?.lua;./test/unit/?.lua;./test/unit/luaunit/?.lua;./luaunit/?.lua'

-- Dependencies
unit = require 'luaunit'

function UTF8Test()
	local utf8 = require("utf8")
	local actual = utf8.len("Hello World!")
	unit.assertEquals(actual, 12)
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

function lutro.filesystem.getSourceBaseDirectoryTest()
	local sourceDir = lutro.filesystem.getSourceBaseDirectory()
	io.write(sourceDir)
	unit.assertIsString(sourceDir)
end

function lutro.getVersionTest()
	local major, minor, revision, codename = lutro.getVersion()
	unit.assertIsNumber(major)
	unit.assertIsNumber(minor)
	unit.assertIsNumber(revision)
	unit.assertEquals(codename, 'Lutro')
end

-- Runs all the defined tests.
function runTests()
	lutro.keyboard.getKeyFromScancodeTest()
	lutro.keyboard.getScancodeFromKeyTest()
	lutro.filesystem.getUserDirectoryTest()
	lutro.filesystem.getSourceBaseDirectoryTest()
	lutro.getVersionTest()
	lutro.math.randomTest()
	lutro.math.setRandomSeedTest()
	UTF8Test()
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
