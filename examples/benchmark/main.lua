function lutro.load()
	sprites = {}
	spriteCount = 0

	spriteImg = lutro.graphics.newImage("logo.png")
	lutro.graphics.setBackgroundColor(54, 172, 248)

	procreate(30,70)
	font = lutro.graphics.newImageFont("font.png", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+/")
	lutro.graphics.setFont(font)
end

function lutro.draw()
	for index,value in ipairs(sprites) do
		local tempSpriteId = value[1]
		local tempSpritePosX = value[2]
		local tempSpritePosY = value[3]

		lutro.graphics.draw(spriteImg, tempSpritePosX, tempSpritePosY)
	end

	lutro.graphics.print("Sprites: " .. spriteCount, 20, 10)
	lutro.graphics.print("FPS " .. lutro.timer.getFPS(), 20, 30)
end

function lutro.update(dt)
	local width, height = spriteImg:getDimensions()
	for index,value in ipairs(sprites) do
		local tempSpriteId = value[1]
		local tempSpritePosX = value[2]
		local tempSpritePosY = value[3]
		local tempSpriteSpeedX = value[4]
		local tempSpriteSpeedY = value[5]

		tempSpritePosX = tempSpritePosX + tempSpriteSpeedX * dt
		tempSpritePosY = tempSpritePosY + tempSpriteSpeedY * dt

		if tempSpritePosX < 0 then
			tempSpriteSpeedX = tempSpriteSpeedX * -1
			tempSpritePosX = 0
		end
		if tempSpritePosY < 0 then
			tempSpriteSpeedY = tempSpriteSpeedY * -1
			tempSpritePosY = 0
		end
		if tempSpritePosX > lutro.graphics.getWidth() - width then
			tempSpriteSpeedX = tempSpriteSpeedX * -1
			tempSpritePosX = lutro.graphics.getWidth() - width
		end
		if tempSpritePosY > lutro.graphics.getHeight() - height then
			tempSpriteSpeedY = tempSpriteSpeedY * -1
			tempSpritePosY = lutro.graphics.getHeight() - height
		end

		local sprite = {tempSpriteId, tempSpritePosX, tempSpritePosY, tempSpriteSpeedX, tempSpriteSpeedY }
		sprites[index] = sprite
	end

	if lutro.joystick.isDown(1, 1) or lutro.joystick.isDown(1, 0) or lutro.joystick.isDown(1, 2) then
		for i = 1, 5 do
			procreate(lutro.math.random(0, lutro.graphics.getWidth()), lutro.math.random(0, lutro.graphics.getHeight()))
		end
	end
end

function procreate(x, y)
	spriteCount = spriteCount + 1
	local spriteId = spriteCount
	local spritePosX = x
	local spritePosY = y
	local maxSpeed = 100
	local spriteSpeedX = lutro.math.random(-maxSpeed, maxSpeed)
	local spriteSpeedY = lutro.math.random(-maxSpeed, maxSpeed)
	local sprite = {spriteId, spritePosX, spritePosY, spriteSpeedX, spriteSpeedY }

	table.insert(sprites, sprite)
end