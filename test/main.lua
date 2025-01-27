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
	print(debug.traceback(errmsg))
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
	g_font = lutro.graphics.newImageFont("graphics/font.png", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+/")

	-- Initiate all available test states.
	-- Do not add states that fail to initialize, to avoid potentially deadlocking errors later on.
	for i, state in ipairs(availableStates) do
		if state then
			local name = "lutro." .. string.gsub(state, "/", ".")
			print (("Test #%d:%s:Import..."):format(i, name))
			local success, test = xpcall(
				function() return require(state) end,
				write_error_traceback
			)
			if not success then
				local msg = ("An error occurred at importing %s"):format(state)
				test = create_failed_test(state, msg)
			end

			test.name = name
			table.insert(states, test)
		end
	end

	-- Load all states.
	for i, state in ipairs(states) do
		print (("Test #%d:%s:Load..."):format(i, state.name))
		if state and state.load then
			local success, test = xpcall(state.load, write_error_traceback)
			if not success then
				local msg = ("An error occurred loading %s"):format(state.name)
				states[i] = create_failed_test(availableStates[i], msg)
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
	if states[currentState] and states[currentState].update then
		local success, test = xpcall(
			function() return states[currentState].update(dt) end,
			get_write_error_traceback(states[currentState].has_update_error)
		)
		if not success then
			-- if an error occurs during update, hook the draw routine to add a message to the display.
			-- this allows the draw to continue even if update fails, as sometimes draws don't explicitly
			-- require updates to work to still report useful results to the developer.
			if not states[currentState].has_update_error then
				local name = states[currentState].name
				local origdraw = states[currentState].draw
				states[currentState].draw = function()
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

	-- setup graphics defaults. Some tests may override this just for the test's render context.
	lutro.graphics.setFont(g_font)
	lutro.graphics.setBackgroundColor(30, 0, 30)
	lutro.graphics.clear()

	-- Draw the current active test
	if states[currentState].draw then
		local success, test = xpcall(states[currentState].draw, get_write_error_traceback(states[currentState].has_draw_error))
		if not success then
			local msg = ("An error occurred drawing %s"):format(states[currentState].name)
			lutro.graphics.print(msg, 30, 35)
			states[currentState].has_draw_error = true
		end
	end

	local status = 'Test ' .. currentState .. ' - ' .. states[currentState].name
	lutro.graphics.print(status, 10, 5)

	-- Testing the keyboard/joystick pressed event.
	lutro.graphics.print(joystickButton, 50, 335)
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
