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

function lutro.filesystem.getUserDirectoryTest()
    local homeDir = lutro.filesystem.getUserDirectory()
    -- @todo Find out how to make os.getenv('HOME') work on Windows?
    local luaHomeDir = os.getenv("HOME")
    unit.assertEquals(homeDir, luaHomeDir)
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

return {
    lutro.filesystem.setRequirePathTest,
    lutro.filesystem.getRequirePathTest,
    lutro.filesystem.getUserDirectoryTest,
    lutro.filesystem.getDirectoryItemsTest
}