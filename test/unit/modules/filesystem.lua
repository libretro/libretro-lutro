function lutro.filesystem.setRequirePathTest()
    local paths = lutro.filesystem.getRequirePath()
    paths = paths .. ';/lutro.filesystem.setRequirePathTest/?.lua'
    lutro.filesystem.setRequirePath(paths)

    local paths = lutro.filesystem.getRequirePath()
    local index = string.find(paths, 'setRequirePathTest')
    unit.assertTrue(index > 1)
end

function lutro.filesystem.getRequirePathTest()
    local paths = lutro.filesystem.getRequirePath()
    unit.assertIsString(paths)
    unit.assertEquals(paths, package.path)
end

function lutro.filesystem.getAppdataDirectoryTest()
    local appdataDir = lutro.filesystem.getAppdataDirectory()
    unit.assertIsString(appdataDir)
end

function lutro.filesystem.getUserDirectoryTest()
    -- UserDirectory should always have a trailing slash. This is easy to verify.
    -- Other aspects of UserDirectory are platform specific and non-trivial to calculate and there
    -- isn't much value to trying to replicate it here.
    local userDir = lutro.filesystem.getUserDirectory()
    local userDirFixed = getUserDir
    if userDirFixed:sub(-1) ~= '/' then
        userDirFixed = userDirFixed .. '/'
    end
    unit.assertEquals(userDir, userDirFixed)
end

function lutro.filesystem.getDirectoryItemsTest()
    -- Get the list of all current directory files.
    local files = lutro.filesystem.getDirectoryItems(".")

    -- Expect to find a main.lua file.
    local foundMainLua = false
    for k, file in ipairs(files) do
        if file == "main.lua" then
            foundMainLua = true
        end
    end
    unit.assertTrue(foundMainLua)
end

function lutro.filesystem.isFileTest()
    unit.assertEquals(lutro.filesystem.isFile("main.lua"), true)
    unit.assertEquals(lutro.filesystem.isFile("missingFile.txt"), false)
end

function lutro.filesystem.isDirectoryTest()
    unit.assertEquals(lutro.filesystem.isDirectory("."), true)
    unit.assertEquals(lutro.filesystem.isDirectory("NotADirectory"), false)
end

return {
    lutro.filesystem.setRequirePathTest,
    lutro.filesystem.getRequirePathTest,
    lutro.filesystem.getUserDirectoryTest,
    lutro.filesystem.getAppdataDirectoryTest,
    lutro.filesystem.getDirectoryItemsTest,
    lutro.filesystem.isFileTest,
    lutro.filesystem.isDirectoryTest
}