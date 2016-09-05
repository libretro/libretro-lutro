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

function LuaSocketTest()
	local http = require('socket.http')
	local result = 'Result'
	local err = 'Request not made yet...'
	result, err = http.request("http://wrong.host/")
	-- @todo Fix LuaSocket test, once LuaSocket works.
	io.write(tostring(err))
end

function lutro.getVersionTest()
	local major, minor, revision, codename = lutro.getVersion()
	unit.assertIsNumber(major)
	unit.assertIsNumber(minor)
	unit.assertIsNumber(revision)
	unit.assertEquals(codename, 'Lutro')
end

function lutro.system.getPowerInfoTest()
	local state, percent, seconds = lutro.system.getPowerInfo()
	unit.assertIsString(state)
	unit.assertEquals(state, 'unknown')
	unit.assertIsNil(percent)
	unit.assertIsNil(seconds)
end

function lutro.system.openURLTest()
	local success = lutro.system.openURL('http://libretro.com')
	unit.assertIsBoolean(success)
	unit.assertEquals(success, false)
end

function lutro.system.vibrate()
	local success = lutro.system.vibrate()
	local success = lutro.system.vibrate(0.5)
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

function lutro.timer.getDeltaTest()
	local delta = lutro.timer.getDelta()
	unit.assertIsNumber(delta)
end

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

-- Runs all the defined tests.
function runTests()
	LuaSocketTest()
	lutro.keyboard.getKeyFromScancodeTest()
	lutro.keyboard.getScancodeFromKeyTest()
	lutro.filesystem.getUserDirectoryTest()
	lutro.getVersionTest()
	lutro.math.randomTest()
	lutro.math.setRandomSeedTest()
	UTF8Test()
	lutro.filesystem.getRequirePathTest()
	lutro.filesystem.setRequirePathTest()

	lutro.getVersionTest()
	lutro.system.getPowerInfoTest()
	lutro.system.openURLTest()
	lutro.system.getProcessorCountTest()
	lutro.system.getClipboardTextTest()
	lutro.system.setClipboardTextTest()

	lutro.timer.getDeltaTest()

	lutro.window.maximizeTest()
	lutro.window.minimizeTest()
	lutro.window.getTitleTest()
	lutro.window.setTitleTest()
	lutro.window.setPositionTest()
	lutro.window.getPositionTest()
	lutro.window.requestAttentionTest()
	lutro.window.getDisplayNameTest()
	lutro.window.setDisplaySleepEnabledTest()
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
