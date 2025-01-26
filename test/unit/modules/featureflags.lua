
local function printBuildFlags()
    -- Step 1: duplicate the table
    -- Step 2: Sort the array alphabetically
    -- Step 3: Iterate over the sorted array

    local copy = {}
    for k in pairs(lutro.featureflags) do
        table.insert(copy, k)
    end

    table.sort(copy, function(a, b)
        return a:lower() < b:lower()
    end)

    -- named prefixed with _ are for internal lutro use.
    for _, key in ipairs(copy) do
        if not key:find("^_") then
            print( ("lutro.featureflags.%s = %s"):format(key, tostring(lutro.featureflags[key])) )
        end
    end
end

local function verify_as_truefalse()
    unit.assertEquals(lutro.featureflags._VERIFY_AS_TRUE,  true)
    unit.assertEquals(lutro.featureflags._VERIFY_AS_FALSE, false)
end

local function verify_as_undefined()
    unit.assertEquals(lutro.featureflags._VERIFY_AS_UNDEFINED, false)
end

return {
    printBuildFlags,
    verify_as_truefalse,
    verify_as_undefined,
}
