-- Add all available include paths.
package.path = package.path .. './test/?.lua;./test/unit/?.lua;./test/unit/luaunit/?.lua;./luaunit/?.lua'

-- Dependencies
unit = require 'luaunit'

-- Runs all tests.
function runTests()
	local moduleTests = {
		require 'modules/filesystem',
		require 'modules/graphics',
		require 'modules/keyboard',
		require 'modules/math',
		require 'modules/system',
		require 'modules/timer',
		require 'modules/window'
	}
	for i, moduleTests in ipairs(moduleTests) do
		for i, functionTest in ipairs(moduleTests) do
			functionTest()
		end
	end
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
