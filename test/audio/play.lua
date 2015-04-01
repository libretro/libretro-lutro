local audio = "audio/test.wav"
local playTime = 0.75
local timer = 0

return {
	load = function()
		audio = lutro.audio.newSource(audio)
	end,

	update = function(dt)
		timer = timer + dt
		if timer > playTime then
			lutro.audio.play(audio)
			timer = 0
		end
	end
}
