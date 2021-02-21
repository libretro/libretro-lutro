function lutro.timer.getDeltaTest()
	local delta = lutro.timer.getDelta()
	unit.assertIsNumber(delta)
end

return {
    lutro.timer.getDeltaTest
}