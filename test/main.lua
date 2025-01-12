-- Lutro Tester
local availableStates = {
	"graphics/print",
	"unit/tests",
	"joystick/isDown",
	"graphics/rectangle",
	"graphics/line",
	"audio/play",
	"joystick/getJoystickCount",
	"window/close"
}
local states = {}
local currentState = 1
local currentTime = 0
local intervalTime = 2.5
local joystickButton = 0
local keypressed = ""

function lutro.load()
	font = lutro.graphics.newImageFont("graphics/font.png", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+/")
	lutro.graphics.setFont(font)
	lutro.graphics.setBackgroundColor(0, 0, 0)

	-- Initiate all available test states.
	for i, state in ipairs(availableStates) do
		local test = require(state)
		test['name'] = "lutro." .. string.gsub(state, "/", ".")
		table.insert(states, test)
	end

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

	-- Testing the keyboard/joystick pressed event.
	lutro.graphics.print(joystickButton, 50, 50)
	lutro.graphics.print(keypressed, 500, 400)

	-- Display the FPS
	local fps = lutro.timer.getFPS()
	lutro.graphics.print('FPS ' .. fps, 10, 350)

	-- Display test instructions.
	text = "Tests switch every 3 seconds. Counting: %.1f"
	lutro.graphics.print(text:format(currentTime), 10,365)
end

function lutro.joystickpressed(joystick, button)
	joystickButton = "Joystick Pressed"
end

function lutro.joystickreleased(joystick, button)
	joystickButton = "Joystick Released"
end

-- Test the Key Pressed event
function lutro.keypressed(key)
	keypressed = key
end

-- Test the Key Released event
function lutro.keyreleased(key)
	keypressed = ''
end
