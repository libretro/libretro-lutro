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

local function write_error_traceback(errmsg)
	print(errmsg)
	print(debug.traceback())
end

local function get_write_error_traceback(noprint)
	return noprint and function() end or write_error_traceback
end


local function create_failed_test(state, msg)
	print( "[TestMain] " .. msg )
	return {
		name = "lutro." .. string.gsub(state, "/", "."),
		draw = function()
			lutro.graphics.print(msg, 30, 20)
		end
	}
end

function lutro.load()
	lutro.graphics.setBackgroundColor(0, 0, 0)

	-- Initiate all available test states.
	-- Do not add states that fail to initialize, to avoid potentially deadlocking errors later on.
	for i, state in ipairs(availableStates) do
		local success, test = xpcall(
			function() return require(state) end,
			write_error_traceback
		)
		if not success then
			local msg = ("An error occurred at importing %s"):format(state)
			test = create_failed_test(state, msg)
		end

		test['name'] = "lutro." .. string.gsub(state, "/", ".")
		table.insert(states, test)
	end

	-- Load all states.
	for i, state in ipairs(states) do
		if states[i] and states[i]['load'] then
			local success, test = xpcall(states[i]['load'], write_error_traceback)
			if not success then
				local msg = ("An error occurred loading %s"):format(state)
				states[i] = create_failed_test(state, msg)
			end
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
	if states[currentState] and states[currentState]['update'] then
		local success, test = xpcall(states[currentState]['update'], get_write_error_traceback(states[currentState].has_update_error), dt)
		if not success then
			-- if an error occurs during update, hook the draw routine to add a message to the display.
			-- this allows the draw to continue even if update fails, as sometimes draws don't explicitly
			-- require updates to work to still report useful results to the developer.
			if not states[currentState].has_update_error then
				local name = states[currentState]['name']
				local origdraw = states[currentState]['draw']
				states[currentState]['draw'] = function()
					local msg = ("An error occurred updating %s"):format(name)
					if origdraw then origdraw() end
					lutro.graphics.print(msg, 30, 20)
				end
			end
			states[currentState].has_update_error = true
		end
	end
end

function lutro.draw()
	if not states[currentState] then
		lutro.graphics.print("Test ".. currentState " - <nil/none/error>", 10, 5)
		return
	end

	-- Draw the current state.
	lutro.graphics.clear()
	local success, test = xpcall(states[currentState]['draw'], get_write_error_traceback(states[currentState].has_draw_error), dt)
	if not success then
		local msg = ("An error occurred drawing %s"):format(states[currentState]['name'])
		lutro.graphics.print(msg, 30, 35)
		states[currentState].has_draw_error = true
	end

	local status = 'Test ' .. currentState .. ' - ' .. states[currentState]['name']
	lutro.graphics.print(status, 10, 5)

	-- Testing the keyboard/joystick pressed event.
	lutro.graphics.print(joystickButton, 50, 335)
	lutro.graphics.print(keypressed, 500, 400)

	-- Display the FPS
	local fps = lutro.timer.getFPS()
	lutro.graphics.print('FPS ' .. fps, 10, 350)
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
