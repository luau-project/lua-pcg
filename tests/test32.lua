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
local pcg32 = pcg.pcg32

print()
print("------------------------------------------")
print("PCG32")
print("------------------------------------------")
print()
print("******************************************")
print()
print("lua-pcg version: " .. pcg.version)
print("32-bit integer: " .. tostring(pcg.has32bitinteger))
print("64-bit integer: " .. tostring(pcg.has64bitinteger))
print("64-bit emulation: " .. tostring(pcg.emulation64bit))

local initstate_pcg32, initseq_pcg32 = '0x853c49e6748fea9b', '0xda3e39cb94b95bdb'
local initstate_pcg32_bytearray = {0x9b, 0xea, 0x8f, 0x74, 0xe6, 0x49, 0x3c, 0x85}
local initseq_pcg32_bytearray = {0xdb, 0x5b, 0xb9, 0x94, 0xcb, 0x39, 0x3e, 0xda}

local function pcg32_100_numbers()
    print()
    print("[PCG32] start 100 numbers")

    local rng = pcg32.new(initstate_pcg32, initseq_pcg32)

    for i = 1, 100 do
        print(("[%03i] 0x%08X"):format(i, rng:next()))
    end

    rng:close()

    print("done")
    print()
end

local function pcg32_100_numbers_from_bytearray()
    print()
    print("[PCG32] start 100 numbers from byte array")

    local rng = pcg32.new(initstate_pcg32_bytearray, initseq_pcg32_bytearray)

    for i = 1, 100 do
        print(("[%03i] 0x%08X"):format(i, rng:next()))
    end

    rng:close()

    print("done")
    print()
end

local function pcg32_random_100_numbers()
    print()
    print("[PCG32] start random 100 numbers")

    local rng = pcg32.new()

    for i = 1, 100 do
        print(("[%03i] 0x%08X"):format(i, rng:next()))
    end

    rng:close()

    print("done")
    print()
end

local function pcg32_random_100_numbers_between_15_40()
    print()
    print("[PCG32] start random 100 numbers between 15 and 40")

    local rng = pcg32.new()
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

local function pcg32_four_bytes()
    print()
    print("[PCG32] start four bytes")

    local rng = pcg32.new(initstate_pcg32, initseq_pcg32)

    local bytes = rng:nextbytes()

    for i, b in ipairs(bytes) do
        print(("[%02i] 0x%02X"):format(i, b))
    end

    rng:close()

    print("done")
    print()
end

local function pcg32_four_bytes_from_bytearray()
    print()
    print("[PCG32] start four bytes from byte array")

    local rng = pcg32.new(initstate_pcg32_bytearray, initseq_pcg32_bytearray)

    local bytes = rng:nextbytes()

    for i, b in ipairs(bytes) do
        print(("[%02i] 0x%02X"):format(i, b))
    end

    rng:close()

    print("done")
    print()
end

local function pcg32_random_four_bytes()
    print()
    print("[PCG32] start random four bytes")

    local rng = pcg32.new()

    local bytes = rng:nextbytes()

    for i, b in ipairs(bytes) do
        print(("[%02i] 0x%02X"):format(i, b))
    end

    rng:close()

    print("done")
    print()
end

local function pcg32_assert_next()
    print()
    print("[PCG32] assert next bytes")

    local rng = pcg32.new(initstate_pcg32, initseq_pcg32)

    local i = 1
    local pcg_file = assert(io.open("tests/next32.txt", "rb"), "Unable to open next32.txt file")
    for line in pcg_file:lines() do
        local bytes = rng:nextbytes()
        if (#bytes ~= 4) then
            error("Unexpected number of bytes provided by pcg32")
        end

        local buffer = {("[%05i] 0x"):format(i)}
        for j = #bytes, 1, -1 do
            table.insert(buffer, ("%02X"):format(bytes[j]))
        end

        if (line ~= table.concat(buffer)) then
            error("Invalid pcg32.next output on line " .. i .. " from next32.txt")
        end

        print(("[%05i] passed"):format(i))
        i = i + 1
    end
    pcg_file:close()

    rng:close()

    print("done")
    print()
end

local function pcg32_assert_next_from_bytearray()
    print()
    print("[PCG32] assert next bytes from byte array")

    local rng = pcg32.new(initstate_pcg32_bytearray, initseq_pcg32_bytearray)

    local i = 1
    local pcg_file = assert(io.open("tests/next32.txt", "rb"), "Unable to open next32.txt file")
    for line in pcg_file:lines() do
        local bytes = rng:nextbytes()
        if (#bytes ~= 4) then
            error("Unexpected number of bytes provided by pcg32")
        end

        local buffer = {("[%05i] 0x"):format(i)}
        for j = #bytes, 1, -1 do
            table.insert(buffer, ("%02X"):format(bytes[j]))
        end

        if (line ~= table.concat(buffer)) then
            error("Invalid pcg32.next output on line " .. i .. " from next32.txt")
        end

        print(("[%05i] passed"):format(i))
        i = i + 1
    end
    pcg_file:close()

    rng:close()

    print("done")
    print()
end

local function pcg32_assert_advance()
    print()
    print("[PCG32] assert advance")

    local rng = pcg32.new(initstate_pcg32, initseq_pcg32)

    local pcg_file = assert(io.open("tests/adv32.txt", "rb"), "Unable to open adv32.txt file")
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
        if (#bytes ~= 4) then
            error("Unexpected number of bytes provided by pcg32")
        end

        local buffer = {("[%05i] delta: %s, next: 0x"):format(i, delta)}
        for j = #bytes, 1, -1 do
            table.insert(buffer, ("%02X"):format(bytes[j]))
        end

        if (line ~= table.concat(buffer)) then
            error("Invalid pcg32.next output on line " .. i .. " from adv32.txt")
        end

        print(("[%05i] passed"):format(i))
    end
    pcg_file:close()

    rng:close()

    print("done")
    print()
end

local tests = {
    pcg32_100_numbers,
    pcg32_100_numbers_from_bytearray,
    pcg32_random_100_numbers,
    pcg32_random_100_numbers_between_15_40,
    pcg32_four_bytes,
    pcg32_four_bytes_from_bytearray,
    pcg32_random_four_bytes,
    pcg32_assert_next,
    pcg32_assert_next_from_bytearray,
    pcg32_assert_advance
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