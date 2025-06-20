-- Allow lua-pcg to be tested
-- without edits on DOSBox
-- (16-bit OS) when lua-pcg is
-- is merged with Lua's
-- interpreter source code (lua.c),
-- To do that, lua.c is edited
-- to load lua-pcg.
if (pcg == nil) then
    pcg = require("lua-pcg")
end
local pcg64 = pcg.pcg64

print()
print("------------------------------------------")
print("PCG64")
print("------------------------------------------")
print()
print("******************************************")
print()
print("lua-pcg version: " .. pcg.version)
print("32-bit integer: " .. tostring(pcg.has32bitinteger))
print("64-bit integer: " .. tostring(pcg.has64bitinteger))
print("64-bit emulation: " .. tostring(pcg.emulation64bit))
print("128-bit emulation: " .. tostring(pcg.emulation128bit))

local initstate_pcg64, initseq_pcg64 = '0x979c9a98d84620057d3e9cb6cfe0549b', '0x0000000000000001da3e39cb94b95bdb'
local initstate_pcg64_bytearray = {0x9b, 0x54, 0xe0, 0xcf, 0xb6, 0x9c, 0x3e, 0x7d, 0x05, 0x20, 0x46, 0xd8, 0x98, 0x9a, 0x9c, 0x97}
local initseq_pcg64_bytearray = {0xdb, 0x5b, 0xb9, 0x94, 0xcb, 0x39, 0x3e, 0xda, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

local function pcg64_100_numbers()
    print()
    print("[PCG64] start 100 numbers")

    local rng = pcg64.new(initstate_pcg64, initseq_pcg64)
    
    for i = 1, 100 do
        print(("[%03i]"):format(i), rng:next())
    end

    rng:close()

    print("done")
    print()
end

local function pcg64_100_numbers_from_bytearray()
    print()
    print("[PCG64] start 100 numbers from byte array")

    local rng = pcg64.new(initstate_pcg64_bytearray, initseq_pcg64_bytearray)

    for i = 1, 100 do
        print(("[%03i]"):format(i), rng:next())
    end

    rng:close()

    print("done")
    print()
end

local function pcg64_random_100_numbers()
    print()
    print("[PCG64] start random 100 numbers")

    local rng = pcg64.new()

    for i = 1, 100 do
        print(("[%03i]"):format(i), rng:next())
    end

    rng:close()

    print("done")
    print()
end

local function pcg64_random_100_numbers_between_15_40()
    print()
    print("[PCG64] start random 100 numbers between 15 and 40")

    local rng = pcg64.new()
    local n

    for i = 1, 100 do
        n = rng:next(15, 40)
        if (not (15 <= n and n < 40)) then
            error(("[%03i] %i is not between 15 and 40"):format(i, n))
        end
        print(("[%03i] %02i"):format(i, n))
    end

    rng:close()

    print("done")
    print()
end

local function pcg64_eight_bytes()
    print()
    print("[PCG64] start eight bytes")

    local rng = pcg64.new(initstate_pcg64, initseq_pcg64)

    local bytes = rng:nextbytes()

    for i, b in ipairs(bytes) do
        print(("[%02i] 0x%02X"):format(i, b))
    end

    rng:close()

    print("done")
    print()
end

local function pcg64_eight_bytes_from_bytearray()
    print()
    print("[PCG64] start eight bytes from byte array")

    local rng = pcg64.new(initstate_pcg64_bytearray, initseq_pcg64_bytearray)
    
    local bytes = rng:nextbytes()

    for i, b in ipairs(bytes) do
        print(("[%02i] 0x%02X"):format(i, b))
    end

    rng:close()

    print("done")
    print()
end

local function pcg64_random_eight_bytes()
    print()
    print("[PCG64] start random eight bytes")

    local rng = pcg64.new()

    local bytes = rng:nextbytes()

    for i, b in ipairs(bytes) do
        print(("[%02i] 0x%02X"):format(i, b))
    end

    rng:close()

    print("done")
    print()
end

local function pcg64_assert_next()
    print()
    print("[PCG64] assert next bytes")

    local rng = pcg64.new(initstate_pcg64, initseq_pcg64)

    local i = 1
    local pcg_file = assert(io.open("tests/next64.txt", "rb"), "Unable to open next64.txt file")
    for line in pcg_file:lines() do
        local bytes = rng:nextbytes()
        if (#bytes ~= 8) then
            error("Unexpected number of bytes provided by pcg64")
        end

        local buffer = {("[%05i] 0x"):format(i)}
        for j = #bytes, 1, -1 do
            table.insert(buffer, ("%02X"):format(bytes[j]))
        end

        if (line ~= table.concat(buffer)) then
            error("Invalid pcg64.next output on line " .. i .. " from next64.txt")
        end

        print(("[%05i] passed"):format(i))
        i = i + 1
    end
    pcg_file:close()

    rng:close()

    print("done")
    print()
end

local function pcg64_assert_next_from_bytearray()
    print()
    print("[PCG64] assert next bytes from byte array")

    local rng = pcg64.new(initstate_pcg64_bytearray, initseq_pcg64_bytearray)

    local i = 1
    local pcg_file = assert(io.open("tests/next64.txt", "rb"), "Unable to open next64.txt file")
    for line in pcg_file:lines() do
        local bytes = rng:nextbytes()
        if (#bytes ~= 8) then
            error("Unexpected number of bytes provided by pcg64")
        end

        local buffer = {("[%05i] 0x"):format(i)}
        for j = #bytes, 1, -1 do
            table.insert(buffer, ("%02X"):format(bytes[j]))
        end

        if (line ~= table.concat(buffer)) then
            error("Invalid pcg64.next output on line " .. i .. " from next64.txt")
        end

        print(("[%05i] passed"):format(i))
        i = i + 1
    end
    pcg_file:close()

    rng:close()

    print("done")
    print()
end

local function pcg64_assert_advance()
    print()
    print("[PCG64] assert advance")

    local rng = pcg64.new(initstate_pcg64, initseq_pcg64)

    local pcg_file = assert(io.open("tests/adv64.txt", "rb"), "Unable to open adv64.txt file")
    for line in pcg_file:lines() do

        local i_str, delta, n = line:match("%[(%d+)%] delta: (0[xX][0-9a-fA-F]+), next: (0[xX][0-9a-fA-F]+)")

        if (i_str == nil) then
            error("line number not found")
        end

        local i = tonumber(i_str)

        if (delta == nil) then
            error("delta not found")
        end

        if (n == nil) then
            error("next not found")
        end

        rng:advance(delta)

        local bytes = rng:nextbytes()
        if (#bytes ~= 8) then
            error("Unexpected number of bytes provided by pcg64")
        end

        local buffer = {("[%05i] delta: %s, next: 0x"):format(i, delta)}
        for j = #bytes, 1, -1 do
            table.insert(buffer, ("%02X"):format(bytes[j]))
        end

        if (line ~= table.concat(buffer)) then
            error("Invalid pcg64.next output on line " .. i .. " from adv64.txt")
        end

        print(("[%05i] passed"):format(i))
    end
    pcg_file:close()

    rng:close()

    print("done")
    print()
end

local tests = {
    pcg64_100_numbers,
    pcg64_100_numbers_from_bytearray,
    pcg64_random_100_numbers,
    pcg64_random_100_numbers_between_15_40,
    pcg64_eight_bytes,
    pcg64_eight_bytes_from_bytearray,
    pcg64_random_eight_bytes,
    pcg64_assert_next,
    pcg64_assert_next_from_bytearray,
    pcg64_assert_advance
}

for i, test_func in ipairs(tests) do
    test_func()
    print("******************************************")
end

print()
print("------------------------------------------")
print("------------------------------------------")
print()
print()