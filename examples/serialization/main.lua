require "global"
require "json"

function love.conf(t)
	t.width  = SCREEN_WIDTH
	t.height = SCREEN_HEIGHT
end

State = {
	x = 50,
	y = 50
}

function love.load()
	love.graphics.setBackgroundColor(0, 0, 0)
	love.graphics.setDefaultFilter("nearest", "nearest")

	IMG_foo = love.graphics.newImage("foo.png")
end

function love.update(dt)
	State.x = State.x + 1
	if State.x > SCREEN_WIDTH then State.x = 0 end
end

function love.draw()
	love.graphics.draw(IMG_foo, State.x, State.y)
end

function love.reset()
	print("reset from lua")

	State = {
		x = 50,
		y = 50
	}

	love.load()
end

function love.serializeSize()
	print("serializeSize")
	return 1024
end

function love.serialize(size)
	print("serialize", size)
	return json_encode(State)
end

function love.unserialize(data, size)
	print("unserialize", data, size)
	State = json_decode(data)
end
