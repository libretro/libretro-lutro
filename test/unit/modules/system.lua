--http = require 'socket.http'

function UTF8Test()
	local utf8 = require("utf8")
	local actual = utf8.len("Hello World!")
	unit.assertEquals(actual, 12)
end

-- function http.requestTest()
--	local http = require('socket.http')
--	local result = http.request('http://buildbot.libretro.com/assets/frontend/info/lutro_libretro.info')
--	unit.assertStrContains(result, 'display_name = "LUA Engine (Lutro)"')
--end

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

function lutro.system.vibrateTest()
	local output = lutro.system.vibrate()
    unit.assertIsNil(output)
	lutro.system.vibrate(0.5)
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

return {
    UTF8Test,
    -- http.requestTest,
    lutro.getVersionTest,
    lutro.system.getPowerInfoTest,
    lutro.system.openURLTest,
    lutro.system.vibrateTest,
    lutro.system.getProcessorCountTest,
    lutro.system.getClipboardTextTest,
    lutro.system.setClipboardTextTest
}