--[[
	Lutro Unit Tests

	These are run through the Lutro testing suite, but you
	can run the unit tests individually by executing:

    retroarch -L path/to/lutro_libretro.so test/unit
]]--

-- Dependency paths.
package.path = package.path .. './test/?.lua;./test/unit/?.lua;./test/unit/luaunit/?.lua;./luaunit/?.lua'

-- Load the unit tests.
require 'tests'

-- Set up Lutro to run the tests at load.
function lutro.load()

	runTests()
	io.write("Lutro unit test run complete\n")
	lutro.event.quit()
end
