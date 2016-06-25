--[[
	Lutro Unit Tests

	These are run through the Lutro testing suite, but you
	can run the unit tests individually by executing:

    retroarch -L path/to/lutro_libretro.so test/unit
]]--

-- LuaUnit
test = require 'luaunit/luaunit'

-- Load the unit tests.
require 'tests'

-- Set up Lutro to run the tests at load.
function lutro.load()
	runTests()
	io.write("Lutro unit test run complete\n")
	lutro.event.quit()
end
