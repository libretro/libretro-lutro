local sound = "audio/test.wav"
local music = "audio/flute.ogg"
local playTime = 1
local timer = 0

return {
	load = function()
		audio = lutro.audio.newSource(sound)
		music = lutro.audio.newSource(music)
		lutro.audio.play(music)
	end,

	update = function(dt)
		timer = timer + dt
		if timer > playTime then
			lutro.audio.play(audio)
			timer = 0
		end
	end
}
