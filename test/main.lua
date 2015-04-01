-- Lutro Tester
local availableStates = {
	"graphics/print",
	"graphics/rectangle",
	"graphics/line",
	"audio/play"
}
local states = {}
local currentState = 1
local currentTime = 0
local intervalTime = 2.5

function lutro.conf(t)
	t.width = 640
	t.height = 480

	-- Initiate all available test states.
	for i, state in ipairs(availableStates) do
		local test = require(state)
		test['name'] = "lutro." .. string.gsub(state, "/", ".")
		table.insert(states, test)
	end
end

function lutro.load()
	lutro.graphics.setBackgroundColor(0, 0, 0)

	-- Load all states.
	for i, state in ipairs(states) do
		if states[i]['load'] then
			states[i]['load']()
		end
	end
end

function lutro.update(dt)
	-- Check if it's time to switch the state.
	currentTime = currentTime + dt
	if currentTime > intervalTime then
		currentTime = 0
		currentState = currentState + 1
		if currentState > #states then
			currentState = 1
		end
	end

	-- Update the current state.
	if states[currentState]['update'] then
		states[currentState]['update'](dt)
	end
end

function lutro.draw()
	-- Draw the current state.
	lutro.graphics.clear()
	if states[currentState]['draw'] then
		states[currentState]['draw']()
	end

	local status = 'Test ' .. currentState .. ' - ' .. states[currentState]['name']
	lutro.graphics.print(status, 10, 5)
end
