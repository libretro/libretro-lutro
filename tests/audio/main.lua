-- global to make for easier debug inspection
g_sources = {}
g_nextId = 1
g_failures = 0

audio_test_name = nil
audio_test_phase_id = 1
last_test_phase_id = 0

-- disable stdout buffering so that prints are in sync with video output.
io.stdout:setvbuf("no")

local s_soundfiles = {
	"test_s16_mono.wav",
	"test_float32_mono.wav",
	"test_ogg_mono.ogg",

	-- test that loading non-existent file does not crash/fail and does not return nil.
	-- expected behavior is that a valid source is returned and can be manipulated same as other sources.
	"fail.wav",
	"fail.ogg"
}

function lutro.load()
	spriteImg = lutro.graphics.newImage("logo.png")
	lutro.graphics.setBackgroundColor(54, 172, 248)

	font = lutro.graphics.newImageFont("font.png", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+/")
	lutro.graphics.setFont(font)

	for i,file in ipairs(s_soundfiles) do
		print ("  loading " .. file)
		g_sources[i] = lutro.audio.newSource(file)
	end
end

function lutro.draw()
	lutro.graphics.draw(spriteImg, 540, 380)

	local numSources = lutro.audio.getActiveSourceCount()
	local sources = lutro.audio.getActiveSources()
	local numPlaying = 0
	local numPaused = 0

	for key, value in ipairs(sources) do
		if value:isPlaying() then numPlaying = numPlaying + 1 end
		if value:isPaused()  then numPaused  = numPaused  + 1 end
	end
	lutro.graphics.print("Running Test: " .. audio_test_name, 20, 10)
	lutro.graphics.print("Active Sources: " .. numSources, 20, 30)
	lutro.graphics.print("  playing: " .. numPlaying, 20, 50)
	lutro.graphics.print("  paused: " .. numPaused, 20, 70)
end

function play_next_sound()
	print(("Playing sound %d: %s"):format(g_nextId, s_soundfiles[g_nextId]))
	local src = lutro.audio.newSource(s_soundfiles[g_nextId])
	src:play()
	-- annoying base-1 index modulo. because lua.
	g_nextId = g_nextId + 1
	g_nextId = ((g_nextId-1) % #s_soundfiles) + 1
	return src
end

function test_non_looping()
	if counter >= 20 then
		counter = 0
		play_next_sound()

		iter = iter + 1
		if iter > 15 then
			return "SUCCESS"
		end
	end
	return 0
end

function verify_cooldown()
	-- cool down to wait for all sounds to stop.
	-- if we add longer-playing sounds to test, this could fail spuriously.
	-- Just increase the delay to 2 secs or whatever. :)

	if counter >= 60 then
		counter = 0
		local numSources = lutro.audio.getActiveSourceCount()
		assert(numSources == 0)
		return "SUCCESS"
	end
end

function test_non_looping_overlap()
	if counter >= 1 then
		counter = 0
		play_next_sound()
		play_next_sound()
		play_next_sound()
		play_next_sound()
		play_next_sound()

		iter = iter + 1
		if iter > 120 then
			return "SUCCESS"
		end
	end
	return 0
end

function test_looping()
	if iter == 15 then
		return "SUCCESS"
	end

	if counter >= 20 then
		counter = 0
		iter = iter + 1

		local src = play_next_sound()
		src:setLooping(true)
		-- looping behavior here is that the sound will "play forever" and GC will be blocked because
		-- the looping flag cannot be removed, since we do not retain a lua reference to the sound.
		-- However, an all-stop of all audio sources should stop the sound. This is what we check for in verify.
	end
end

function verify_looprun()
	if counter >= 60 then
		counter = 0

		local sources = lutro.audio.getActiveSources()

		for key,value in pairs(sources) do
			value:setLooping(false)
		end

		-- missing and unsupported wavfiles should not loop, which is why sources != iter
		if #sources ~= 6 then
			print ("verify_looprun FAILED")
			print ((" sources: %d"):format(#sources))
			return "FAILURE"
		end

		return "SUCCESS"
	end
end

function test_source_pause()
	if iter == 15 then
		return "SUCCESS"
	end

	if counter >= 20 then
		counter = 0
		iter = iter + 1

		local src = play_next_sound()
		src:pause(true)
	end
end

function verify_pauserun()
	if counter >= 60 then
		counter = 0

		local sources = lutro.audio.getActiveSources()

		for key,value in pairs(sources) do
			assert(value:isPaused())
			value:setLooping(false)
			value:play(false)
		end

		-- missing and unsupported wavfiles should not loop, which is why sources != iter
		if #sources ~= 6 then
			print ("verify_pauserun FAILED")
			print ((" sources: %d"):format(#sources))
			return "FAILURE"
		end

		return "SUCCESS"
	end
end

function test_audio_pause()
	if iter == 15 then
		lutro.audio.pause()
		return "SUCCESS"
	end

	if counter >= 60 then
		counter = 0
		iter = iter + 1

		local src = play_next_sound()
		src:setLooping(true)
	end
end

audio_test_phases = {
	"test_non_looping",
	"verify_cooldown",

	"test_non_looping_overlap",
	"verify_cooldown",

	"test_looping",
	"verify_looprun",
	"verify_cooldown",

	"test_source_pause",
	"verify_pauserun",
	"verify_cooldown",

	"test_audio_pause",
	"verify_pauserun",
	"verify_cooldown",
	nil
}

counter = 0
iter = 0

function string.startswith(String,Start)
   return string.sub(String, 1, string.len(Start)) == Start
end

function lutro.update(dt)
	counter = counter + 1

	local test_function = audio_test_phases[audio_test_phase_id];
	if test_function == nil then
		if g_failures then
			print(("FINISHED with %d FAILURES."):format(g_failures))
		else
			print(("FINISHED with FULL SUCCESS."))
		end
		lutro.event.quit()
		return
	end

	if last_test_phase_id ~= audio_test_phase_id then
		last_test_phase_id = audio_test_phase_id
		local action_type = "RUN"

		if test_function:startswith("verify_") then
			action_type = "VERIFY"
		end

		if action_type == "RUN" then
			print("====================================================================")
			audio_test_name = test_function
		end
		print(("== %s : %s"):format(action_type, test_function))
	end

	local result = _G[test_function]();
	if result then
		print (("%s:%s"):format(result, audio_test_name))
		if result == "FAILURE" then
			g_failures = g_failures + 1
		end

		audio_test_phase_id = audio_test_phase_id + 1
		counter = 0
		iter = 0
		g_nextId = 1
	end
end
