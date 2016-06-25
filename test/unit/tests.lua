-- Load LuaUnit
unit = require 'test/unit/luaunit/luaunit'

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

function LuaSocketTest()
	local http = require('socket.http')
	local result = 'Result'
	local err = 'Request not made yet...'
	result, err = http.request("http://wrong.host/")
	-- @todo Fix LuaSocket.
	io.write(tostring(err))
end

-- Runs all the defined tests.
function runTests()
	lutro.math.setRandomSeedTest()
	lutro.math.randomTest()
	LuaSocketTest()
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
