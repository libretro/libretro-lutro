local m_is_unattended = false

return {
	load = function()
		-- When deciding to terminate the process, check for "UNATTENDED" env var. This is preferred over
		-- "CI" as the name is more explicit. "CI" is short and likely to have other meanings in casual
		-- user/developer environments and should be avoided when making potentially surprising actions
		-- like sudden process termination.
		local env_ci = os.getenv("UNATTENDED")
		if env_ci then
			print ("env: UNATTENDED=" .. env_ci)
			env_ci = env_ci:lower()
			-- consider any value other than something specifically "false" as a truthy value.
			if env_ci ~= "false" and env_ci ~= "0" and env_ci ~= "no" then
				m_is_unattended = true
			end
		end
	end,

	draw = function()
		-- test that window.close() operates normally, but also terminate the process via lua to
		-- force stop the test even if the libretro frontend is designed or configured to be persistent.
		-- This allows using the tests via some automatic PR sweeps.

		print ("All Tests Complete. Closing libretro frontend window...")
		lutro.window.close()

		if m_is_unattended then
			print (" > Exiting process due to env UNATTENDED=" .. os.getenv("UNATTENDED"))
			os.exit()
		else
			print ("Note: Libretro frontend may continue running depending on mode/settings.")
			print (" > Set UNATTENDED=1 in the process environment to exit the libretro frontend immediately.")
		end
	end
}
