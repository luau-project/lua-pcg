# lua-pcg

> [!IMPORTANT]
> 
> **DO NOT** use this library while it is not published on LuaRocks. Currently, it is under active development, and the last bugs are being fixed.

[![CI](https://github.com/luau-project/lua-pcg/actions/workflows/ci.yml/badge.svg)](./.github/workflows/ci.yml)
<!--[![LuaRocks](https://img.shields.io/luarocks/v/luau-project/lua-pcg?label=LuaRocks&color=2c3e67)](https://luarocks.org/modules/luau-project/lua-pcg)-->

## Overview

Written in C89, **lua-pcg** implements methods of the PCG algorithms for Lua. **P**ermuted **C**ongruential **G**enerator (PCG) is a family of simple fast space-efficient statistically good algorithms designed in 2014 by Dr. M.E. O'Neill for random number generation. For a detailed explanation about PCG, visit the authors website [https://www.pcg-random.org/](https://www.pcg-random.org/).

On old C compilers that do not provide support to 64-bit or 128-bit integers, `lua-pcg` performs integer arithmetic on software to implement PCG algorithms. If you find any bug on `lua-pcg`, being aware of the [known limitations](#known-limitations), feel free to open issues.

> [!NOTE]
> 
> **Fun fact**: Did you know that `lua-pcg` runs on DOS? Check it out [here](./docs/DOSBox.md)!

## Table of Contents

* [Installation](#installation)
* [Usage](#usage)
    * [pcg32](#pcg32)
        * [Generate a pseudo random 32-bit integer](#generate-a-pseudo-random-32-bit-integer)
        * [Generate a bounded pseudo random 32-bit integer](#generate-a-bounded-pseudo-random-32-bit-integer)
        * [Generate a pseudo random 32-bit integer on interval](#generate-a-pseudo-random-32-bit-integer-on-interval)
        * [Generate the bytes of a pseudo random 32-bit integer](#generate-the-bytes-of-a-pseudo-random-32-bit-integer)
        * [Seed the pcg32 random number generator](#seed-the-pcg32-random-number-generator)
    * [pcg64](#pcg64)
        * [Generate a pseudo random 64-bit integer](#generate-a-pseudo-random-64-bit-integer)
        * [Generate a bounded pseudo random 64-bit integer](#generate-a-bounded-pseudo-random-64-bit-integer)
        * [Generate a pseudo random 64-bit integer on interval](#generate-a-pseudo-random-64-bit-integer-on-interval)
        * [Generate the bytes of a pseudo random 64-bit integer](#generate-the-bytes-of-a-pseudo-random-64-bit-integer)
        * [Seed the pcg64 random number generator](#seed-the-pcg64-random-number-generator)
* [Properties](#properties)
    * [emulation128bit](#emulation128bit)
    * [emulation64bit](#emulation64bit)
    * [has32bitinteger](#has32bitinteger)
    * [has64bitinteger](#has64bitinteger)
* [Classes](#classes)
    * [pcg32](#pcg32-1)
        * [advance](#advance)
        * [close](#close)
        * [new](#new)
        * [next](#next)
        * [nextbytes](#nextbytes)
        * [seed](#seed)
    * [pcg64](#pcg64-1)
        * [advance](#advance-1)
        * [close](#close-1)
        * [new](#new-1)
        * [next](#next-1)
        * [nextbytes](#nextbytes-1)
        * [seed](#seed-1)
* [Known limitations](#known-limitations)
* [Change log](#change-log)

## Installation

`lua-pcg` is dependency-free. All you need is a working installation of Lua, and the same C toolchain used to build Lua.

### LuaRocks

By far, [LuaRocks](https://luarocks.org) provides the easiest manner to build and install `lua-pcg` on your machine. Assuming that `LuaRocks` is properly installed and configured on your system, execute the following command:

```bash
luarocks install lua-pcg
```

### Alternative installation methods

Are you looking to install `lua-pcg` out of LuaRocks? Check the [docs](./docs/README.md).

## Usage

In order to understand the usage, we split it on two flavours: `pcg32` and `pcg64`. To keep it simple in short terms, the `pcg32` class is employed to generate pseudo random 32-bit integers, while the `pcg64` class is meant for pseudo random 64-bit integers.

### pcg32

The `pcg32` class allows the Lua programmer to generate pseudo random 32-bit integers ($2^{64}$ period).

#### Generate a pseudo random 32-bit integer

For each iteration, we generate a 32-bit integer $n$.

```lua
-- load the library
local pcg = require("lua-pcg")

-- create a pcg32 instance
local rng = pcg.pcg32.new()

-- iterate ten times
for i = 1, 10 do

    -- n is a pseudo random 32-bit integer
    local n = rng:next()

    -- prints the generated n
    print(i, n)
end

-- free resources
-- 
-- tip: resources are 
-- automatically freed
-- at garbage collection
rng:close()
```

#### Generate a bounded pseudo random 32-bit integer

For each iteration, we generate a 32-bit integer $n$ such that $0 \leq n < 30$.

```lua
-- load the library
local pcg = require("lua-pcg")

-- create a pcg32 instance
local rng = pcg.pcg32.new()

-- iterate ten times
for i = 1, 10 do

    -- n is a pseudo random 32-bit integer
    -- such that 0 <= n and n < 30
    local n = rng:next(30)

    -- prints the generated n
    print(i, n)
end

-- free resources
-- 
-- tip: resources are 
-- automatically freed
-- at garbage collection
rng:close()
```

#### Generate a pseudo random 32-bit integer on interval

For this example, on each iteration, we generate a 32-bit integer $n$ such that $-50 \leq n < 30$.

```lua
-- load the library
local pcg = require("lua-pcg")

-- create a pcg32 instance
local rng = pcg.pcg32.new()

-- iterate ten times
for i = 1, 10 do

    -- n is a pseudo random 32-bit integer
    -- such that -50 <= n and n < 30
    local n = rng:next(-50, 30)

    -- prints the generated n
    print(i, n)
end

-- free resources
-- 
-- tip: resources are 
-- automatically freed
-- at garbage collection
rng:close()
```

#### Generate the bytes of a pseudo random 32-bit integer

This time, we get the four bytes of a pseudo random 32-bit integer

```lua
-- load the library
local pcg = require("lua-pcg")

-- create a pcg32 instance
local rng = pcg.pcg32.new()

-- gets the bytes of a pseudo random 32-bit integer
local bytes = rng:nextbytes()

assert(#bytes == 4, "Unexpected number of bytes in a 32-bit integer")

-- print each byte 'b' in hex format
for i, b in ipairs(bytes) do
    print(("[%i] 0x%02X"):format(i, b))
end

-- free resources
-- 
-- tip: resources are 
-- automatically freed
-- at garbage collection
rng:close()
```

#### Seed the pcg32 random number generator

For reproducible sequences, you are required to provide two (2) parameters (`initstate` and `initseq`), each one as a 64-bit integer written as a string following the regex pattern `0[xX][0-9a-fA-F]{1,16}` (e.g.: `0xae9bd64ed8e0074a`), or as a byte array (e.g.: `{0x4a, 0x07, 0xe0, 0xd8, 0x4e, 0xd6, 0x9b, 0xae}` in little-endian byte order).

```lua
-- load the library
local pcg = require("lua-pcg")

-- define initstate for reproducible builds
local initstate = '0x853c49e6748fea9b'

-- the same value 'initstate' above could be
-- provided as the following byte array in little-endian byte order:
-- {0x9b, 0xea, 0x8f, 0x74, 0xe6, 0x49, 0x3c, 0x85}

-- define initseq for reproducible builds
local initseq = '0xda3e39cb94b95bdb'

-- the same value 'initseq' above could be
-- provided as the following byte array in little-endian byte order:
-- {0xdb, 0x5b, 0xb9, 0x94, 0xcb, 0x39, 0x3e, 0xda}

-- create a pcg32 instance
local rng = pcg.pcg32.new(initstate, initseq)

-- iterate ten times
for i = 1, 10 do

    -- n is a pseudo random 32-bit integer
    local n = rng:next()

    -- prints the generated n
    print(i, n)
end

-- free resources
-- 
-- tip: resources are 
-- automatically freed
-- at garbage collection
rng:close()
```

> [!NOTE]
> 
> Running the code above, you should always see the same exact sequence of integers printed.

> [!TIP]
> 
> Without any arguments, the default `pcg32` constructor uses a rudimentary (*weak*) approach based on time, and addresses of variables, in order to generate different seeds to bootstrap the 32-bit RNG. As a recommended strategy to enhance the process to obtain random seeds, a **C**ryptographically **S**ecure **P**seudo **R**andom **N**umber **G**enerator (CSPRNG) can be employed to generate the seeds `initstate` and `initseq` (check the `bytes` method of our [lua-cryptorandom](https://github.com/luau-project/lua-cryptorandom#bytes)) library.

[Back to TOC](#table-of-contents)

### pcg64

The `pcg64` class allows the Lua programmer to generate pseudo random 64-bit integers ($2^{128}$ period).

#### Generate a pseudo random 64-bit integer

For each iteration, we generate a 64-bit integer $n$.

```lua
-- load the library
local pcg = require("lua-pcg")

-- create a pcg64 instance
local rng = pcg.pcg64.new()

-- iterate ten times
for i = 1, 10 do

    -- n is a pseudo random 64-bit integer
    local n = rng:next()

    -- prints the generated n
    print(i, n)
end

-- free resources
-- 
-- tip: resources are 
-- automatically freed
-- at garbage collection
rng:close()
```

#### Generate a bounded pseudo random 64-bit integer

For each iteration, we generate a 64-bit integer $n$ such that $0 \leq n < 30$.

```lua
-- load the library
local pcg = require("lua-pcg")

-- create a pcg64 instance
local rng = pcg.pcg64.new()

-- iterate ten times
for i = 1, 10 do

    -- n is a pseudo random 64-bit integer
    -- such that 0 <= n and n < 30
    local n = rng:next(30)

    -- prints the generated n
    print(i, n)
end

-- free resources
-- 
-- tip: resources are 
-- automatically freed
-- at garbage collection
rng:close()
```

#### Generate a pseudo random 64-bit integer on interval

For this example, on each iteration, we generate a 64-bit integer $n$ such that $-50 \leq n < 30$.

```lua
-- load the library
local pcg = require("lua-pcg")

-- create a pcg64 instance
local rng = pcg.pcg64.new()

-- iterate ten times
for i = 1, 10 do

    -- n is a pseudo random 64-bit integer
    -- such that -50 <= n and n < 30
    local n = rng:next(-50, 30)

    -- prints the generated n
    print(i, n)
end

-- free resources
-- 
-- tip: resources are 
-- automatically freed
-- at garbage collection
rng:close()
```

#### Generate the bytes of a pseudo random 64-bit integer

This time, we get the eight bytes of a pseudo random 64-bit integer

```lua
-- load the library
local pcg = require("lua-pcg")

-- create a pcg64 instance
local rng = pcg.pcg64.new()

-- gets the bytes of a pseudo random 64-bit integer
local bytes = rng:nextbytes()

assert(#bytes == 8, "Unexpected number of bytes in a 64-bit integer")

-- print each byte 'b' in hex format
for i, b in ipairs(bytes) do
    print(("[%i] 0x%02X"):format(i, b))
end

-- free resources
-- 
-- tip: resources are 
-- automatically freed
-- at garbage collection
rng:close()
```

#### Seed the pcg64 random number generator

For reproducible sequences, you are required to provide two (2) parameters (`initstate` and `initseq`), each one as a 128-bit integer written as a string following the regex pattern `0[xX][0-9a-fA-F]{1,32}` (e.g.: `0x21801e8b90be2aa5d7f621b1c4c1301b`), or as a byte array (e.g.: `{0x1b, 0x30, 0xc1, 0xc4, 0xb1, 0x21, 0xf6, 0xd7, 0xa5, 0x2a, 0xbe, 0x90, 0x8b, 0x1e, 0x80, 0x21}` in little-endian byte order).

```lua
-- load the library
local pcg = require("lua-pcg")

-- define initstate for reproducible builds
local initstate = '0x979c9a98d84620057d3e9cb6cfe0549b'

-- the same value 'initstate' above could be
-- provided as the following byte array in little-endian byte order:
-- {0x9b, 0x54, 0xe0, 0xcf, 0xb6, 0x9c, 0x3e, 0x7d, 0x05, 0x20, 0x46, 0xd8, 0x98, 0x9a, 0x9c, 0x97}

-- define initseq for reproducible builds
local initseq = '0x0000000000000001da3e39cb94b95bdb'

-- the same value 'initseq' above could be
-- provided as the following byte array in little-endian byte order:
-- {0xdb, 0x5b, 0xb9, 0x94, 0xcb, 0x39, 0x3e, 0xda, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

-- create a pcg64 instance
local rng = pcg.pcg64.new(initstate, initseq)

-- iterate ten times
for i = 1, 10 do

    -- n is a pseudo random 64-bit integer
    local n = rng:next()

    -- prints the generated n
    print(i, n)
end

-- free resources
-- 
-- tip: resources are 
-- automatically freed
-- at garbage collection
rng:close()
```

> [!NOTE]
> 
> Running the code above, you should always see the same exact sequence of integers printed.

> [!TIP]
> 
> Without any arguments, the default `pcg64` constructor uses a rudimentary (*weak*) approach based on time, and addresses of variables, in order to generate different seeds to bootstrap the 64-bit RNG. As a recommended strategy to enhance the process to obtain random seeds, a CSPRNG can be employed to generate the seeds `initstate` and `initseq` (check the `bytes` method of our [lua-cryptorandom](https://github.com/luau-project/lua-cryptorandom#bytes)) library.

[Back to TOC](#table-of-contents)

## Properties

### emulation128bit

* *Description*: Determines whether `lua-pcg` is emulating 128-bit integers on software or not.
* *Signature*: `emulation128bit`
    * *Return* (`boolean`): a flag to tell whether `lua-pcg` is emulating 128-bit integers on software or not.
* *Remark*: 128-bit integer support is not present on C89 by default. When `lua-pcg` is built in the absence of 128-bit integers, it emulates 128-bit integers on software to provide the underlying features of PCG.
* *Usage*:
    ```lua
    local pcg = require("lua-pcg")

    print('lua-pcg is emulating 128-bit integers on software:', pcg.emulation128bit)
    ```

### emulation64bit

* *Description*: Determines whether `lua-pcg` is emulating 64-bit integers on software or not.
* *Signature*: `emulation64bit`
    * *Return* (`boolean`): a flag to tell whether `lua-pcg` is emulating 64-bit integers on software or not.
* *Remark*: 64-bit integer support is not present on C89 by default. When `lua-pcg` is built in the absence of 64-bit integers, it emulates 64-bit integers on software to provide the underlying features of PCG.
* *Usage*:
    ```lua
    local pcg = require("lua-pcg")

    print('lua-pcg is emulating 64-bit integers on software:', pcg.emulation64bit)
    ```

> [!NOTE]
> 
> The emulation of integer operations on software is slower compared to integer support on hardware.

### has32bitinteger

* *Description*: Determines whether the Lua type `lua_Integer` has at least 32-bits or not.
* *Signature*: `has32bitinteger`
    * *Return* (`boolean`): a flag to tell whether the Lua type `lua_Integer` has at least 32-bits or not.
* *Remark*:
    1. `lua-pcg` is able to run even on a 16-bit operating system. In a 16-bit operating system like DOS, Windows 3.1, etc, the type `lua_Integer` on Lua 5.1 has 16-bits. The primary purpose of this flag is to tell when the output of the methods [next (pcg32)](#next) and [next (pcg64)](#next-1) are including 32-bits of the integer.
    2. Moreover, see [known limitations](#known-limitations).
* *Usage*:
    ```lua
    local pcg = require("lua-pcg")

    print('integers on Lua has at least 32-bits:', pcg.has32bitinteger)

    -- create a pcg32 instance
    local rng32 = pcg.pcg32.new()

    -- generate a random integer
    local n32 = rng32:next()

    if (pcg.has32bitinteger) then
        -- n32 contains the whole 32-bits
        -- of the random 32-bit integer

        -- However, be aware of the known limitations
        print("the generated value contains all the 32-bits:", n32)
    else
        -- n32 is casted to 'lua_Integer',
        -- which contains less than
        -- 32-bits.
        print("the generated value was truncated:", n32)
    end
    ```

### has64bitinteger

* *Description*: Determines whether the Lua type `lua_Integer` has at least 64-bits or not.
* *Signature*: `has64bitinteger`
    * *Return* (`boolean`): a flag to tell whether the Lua type `lua_Integer` has at least 64-bits or not.
* *Remark*:
    1. On 32-bit operating system like Windows XP, Debian 32-bits, etc, the type `lua_Integer` on Lua 5.1 has 32-bits. The primary purpose of this flag is to tell when the output of the method [next (pcg64)](#next-1) is including 64-bits of the integer.
    2. Moreover, see [known limitations](#known-limitations).
* *Usage*:
    ```lua
    local pcg = require("lua-pcg")

    print('integers on Lua has at least 64-bits:', pcg.has64bitinteger)

    -- create a pcg64 instance
    local rng64 = pcg.pcg64.new()

    -- generate a random integer
    local n64 = rng64:next()

    if (pcg.has64bitinteger) then
        -- n64 contains the whole 64-bits
        -- of the random 64-bit integer

        -- However, be aware of the known limitations
        print("the generated value contains all the 64-bits:", n64)
    else
        -- n64 is casted to 'lua_Integer',
        -- which contains less than
        -- 64-bits.
        print("the generated value was truncated:", n64)
    end
    ```

### version

* *Description*: The version of this library
* *Signature*: `version`
    * *Return* (`string`): a string containing the version of this library (e.g.: `0.0.1`).
* *Usage*:
    ```lua
    local pcg = require("lua-pcg")

    -- prints the version of lua-pcg
    print(pcg.version)
    ```

[Back to TOC](#table-of-contents)

## Classes

The description of methods are split on two classes: `pcg32` and `pcg64`.

### pcg32

This class is able to generate pseudo random 32-bit integers and their four bytes.

#### advance

* *Description*: Multi-step advance functions (jump-ahead, jump-back)
* *Signature*: `rng:advance(delta)`
    * *Parameters*:
        * *rng* (`userdata`): an instance of the [pcg32](#pcg32-1) class;
        * *delta* (`string | table`): 64-bit integer written as a `string` following the regex pattern `0[xX][0-9a-fA-F]{1,16}` (e.g.: `0xae9bd64ed8e0074a`), or a `table` representing the 64-bit integer as a byte array (e.g.: `{0x4a, 0x07, 0xe0, 0xd8, 0x4e, 0xd6, 0x9b, 0xae}` in little-endian byte order).
    * *Return* (`void`).

#### close

* *Description*: Frees resources held by the `pcg32` rng instance.
* *Signature*: `rng:close()`
    * *Parameters*:
        * *rng* (`userdata`): an instance of the [pcg32](#pcg32-1) class;
    * *Return* (`void`).

#### new

* *Description*: Initializes an instance of the [pcg32](#pcg32-1) class according to optionally provided parameters `initstate` and `initseq`.
* *Signature*: `rng:new([initstate [, initseq]])`
    * *Parameters*:
        * *rng* (`userdata`): an instance of the [pcg32](#pcg32-1) class;
        * *initstate* (`string | table`): 64-bit integer written as a `string` following the regex pattern `0[xX][0-9a-fA-F]{1,16}` (e.g.: `0x853c49e6748fea9b`), or a `table` representing the 64-bit integer as a byte array (e.g.: `{0x9b, 0xea, 0x8f, 0x74, 0xe6, 0x49, 0x3c, 0x85}` in little-endian byte order);
        * *initseq* (`string | table`): 64-bit integer written as a `string` following the regex pattern `0[xX][0-9a-fA-F]{1,16}` (e.g.: `0xda3e39cb94b95bdb`), or a `table` representing the 64-bit integer as a byte array (e.g.: `{0xdb, 0x5b, 0xb9, 0x94, 0xcb, 0x39, 0x3e, 0xda}` in little-endian byte order).
    * *Remark*: When any of the optional parameters is not provided, `lua-pcg` generates them through a rudimentary (*weak*) approach based on time, and addresses of variables, in order to bootstrap the 32-bit RNG. As a recommended strategy to enhance the process to obtain random seeds, a CSPRNG can be employed to generate the seeds `initstate` and `initseq` (check the `bytes` method of our [lua-cryptorandom](https://github.com/luau-project/lua-cryptorandom#bytes)) library.
    * *Return* (`void`).

#### next

* *Description*: Gets the next 32-bit integer provided by the `rng` instance of [pcg32](#pcg32-1).
* *Signature*: `rng:next([a [, b]])`
    * *Parameters*:
        * *rng* (`userdata`): an instance of the [pcg32](#pcg32-1) class;
        * *a* (`integer`):
            * when `a` is provided and `b` is not, a value $n$ is generated such that $0 \leq n < a$;
            * when `a` and `b` are provided, then a value $n$ is generated such that $a \leq n < b$.
        * *b* (`integer`): the maximum (exclusive) value allowed for the generated value.
    * *Remark*: 
        1. The 32-bit integer generated by `rng` is casted to fit on a Lua integer. Be aware that the vanilla builds of Lua 5.1 and 5.2 on 16-bit operating systems, this value is truncated to 16-bit;
        2. Moreover, see [known limitations](#known-limitations).
    * *Exceptions*:
        * if only `a` is provided, then an exception is thrown when `a` is out of [1, 4294967295] interval.
        * if `a` and `b` are provided, then an exception is thrown when the difference `b - a` is out of [1, 4294967295] interval.
    * *Return* (`integer`): the generated Lua integer.

#### nextbytes

* *Description*: Gets the bytes of the next 32-bit integer provided by the `rng` instance of [pcg32](#pcg32-1).
* *Signature*: `rng:nextbytes()`
    * *Parameters*:
        * *rng* (`userdata`): an instance of the [pcg32](#pcg32-1) class;
    * *Return* (`table`): a table containing exactly 4 bytes, such that a byte a meant as an integer on 0 -- 255 range.

#### seed

* *Description*: Sets the seeds (`initstate` and `initseq`) of the `rng` instance of [pcg32](#pcg32-1).
* *Signature*: `rng:seed(initstate, initseq)`
    * *Parameters*:
        * *rng* (`userdata`): an instance of the [pcg32](#pcg32-1) class;
        * *initstate* (`string | table`): 64-bit integer written as a `string` following the regex pattern `0[xX][0-9a-fA-F]{1,16}` (e.g.: `0x853c49e6748fea9b`), or a `table` representing the 64-bit integer as a byte array (e.g.: `{0x9b, 0xea, 0x8f, 0x74, 0xe6, 0x49, 0x3c, 0x85}` in little-endian byte order);
        * *initseq* (`string | table`): 64-bit integer written as a `string` following the regex pattern `0[xX][0-9a-fA-F]{1,16}` (e.g.: `0xda3e39cb94b95bdb`), or a `table` representing the 64-bit integer as a byte array (e.g.: `{0xdb, 0x5b, 0xb9, 0x94, 0xcb, 0x39, 0x3e, 0xda}` in little-endian byte order).
    * *Return* (`void`).

[Back to TOC](#table-of-contents)

### pcg64

This class is able to generate pseudo random 64-bit integers and their eight bytes.

#### advance

* *Description*: Multi-step advance functions (jump-ahead, jump-back)
* *Signature*: `rng:advance(delta)`
    * *Parameters*:
        * *rng* (`userdata`): an instance of the [pcg64](#pcg64-1) class;
        * *delta* (`string | table`): 128-bit integer written as a `string` following the regex pattern `0[xX][0-9a-fA-F]{1,16}` (e.g.: `0x21801e8b90be2aa5d7f621b1c4c1301b`), or a `table` representing the 128-bit integer as a byte array (e.g.: `{0x1b, 0x30, 0xc1, 0xc4, 0xb1, 0x21, 0xf6, 0xd7, 0xa5, 0x2a, 0xbe, 0x90, 0x8b, 0x1e, 0x80, 0x21}` in little-endian byte order).
    * *Return* (`void`).

#### close

* *Description*: Frees resources held by the `pcg64` rng instance.
* *Signature*: `rng:close()`
    * *Parameters*:
        * *rng* (`userdata`): an instance of the [pcg64](#pcg64-1) class;
    * *Return* (`void`).

#### new

* *Description*: Initializes an instance of the [pcg64](#pcg64-1) class according to optionally provided parameters `initstate` and `initseq`.
* *Signature*: `rng:new([initstate [, initseq]])`
    * *Parameters*:
        * *rng* (`userdata`): an instance of the [pcg64](#pcg64-1) class;
        * *initstate* (`string | table`): 128-bit integer written as a `string` following the regex pattern `0[xX][0-9a-fA-F]{1,32}` (e.g.: `0x979c9a98d84620057d3e9cb6cfe0549b`), or a `table` representing the 128-bit integer as a byte array (e.g.: `{0x9b, 0x54, 0xe0, 0xcf, 0xb6, 0x9c, 0x3e, 0x7d, 0x05, 0x20, 0x46, 0xd8, 0x98, 0x9a, 0x9c, 0x97}` in little-endian byte order);
        * *initseq* (`string | table`): 128-bit integer written as a `string` following the regex pattern `0[xX][0-9a-fA-F]{1,32}` (e.g.: `0x0000000000000001da3e39cb94b95bdb`), or a `table` representing the 128-bit integer as a byte array (e.g.: `{0xdb, 0x5b, 0xb9, 0x94, 0xcb, 0x39, 0x3e, 0xda, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}` in little-endian byte order).
    * *Remark*: When any of the optional parameters is not provided, `lua-pcg` generates them through a rudimentary (*weak*) approach based on time, and addresses of variables, in order to bootstrap the 64-bit RNG. As a recommended strategy to enhance the process to obtain random seeds, a CSPRNG can be employed to generate the seeds `initstate` and `initseq` (check the `bytes` method of our [lua-cryptorandom](https://github.com/luau-project/lua-cryptorandom#bytes)) library.
    * *Return* (`void`).

#### next

* *Description*: Gets the next 64-bit integer provided by the `rng` instance of [pcg64](#pcg64-1).
* *Signature*: `rng:next([a [, b]])`
    * *Parameters*:
        * *rng* (`userdata`): an instance of the [pcg64](#pcg64-1) class;
        * *a* (`integer`):
            * when `a` is provided and `b` is not, a value $n$ is generated such that $0 \leq n < a$;
            * when `a` and `b` are provided, then a value $n$ is generated such that $a \leq n < b$.
        * *b* (`integer`): the maximum (exclusive) value allowed for the generated value.
    * *Remark*: 
        1. The 64-bit integer generated by `rng` is casted to fit on a Lua integer. Be aware that the vanilla builds of Lua 5.1 and 5.2 on 32-bit operating systems, this value is truncated to 32-bit. Moreover, Lua 5.3 and 5.4 allows the integer to be 32-bits. Everytime the Lua type `lua_Integer` is shorter than 64-bits in size, the generated value is truncated to fit on a `lua_Integer`;
        2. Moreover, see [known limitations](#known-limitations).

    * *Exceptions*:
        * if `a` and `b` are provided, then an exception is thrown when $a \geq b$.
    * *Return* (`integer`): the generated Lua integer.

#### nextbytes

* *Description*: Gets the bytes of the next 64-bit integer provided by the `rng` instance of [pcg64](#pcg64-1).
* *Signature*: `rng:nextbytes()`
    * *Parameters*:
        * *rng* (`userdata`): an instance of the [pcg64](#pcg64-1) class;
    * *Return* (`table`): a table containing exactly 8 bytes, such that a byte a meant as an integer on 0 - 255 range.

#### seed

* *Description*: Sets the seeds (`initstate` and `initseq`) of the `rng` instance of [pcg64](#pcg64-1).
* *Signature*: `rng:seed(initstate, initseq)`
    * *Parameters*:
        * *rng* (`userdata`): an instance of the [pcg64](#pcg64-1) class;
        * *initstate* (`string | table`): 128-bit integer written as a `string` following the regex pattern `0[xX][0-9a-fA-F]{1,32}` (e.g.: `0x979c9a98d84620057d3e9cb6cfe0549b`), or a `table` representing the 128-bit integer as a byte array (e.g.: `{0x9b, 0x54, 0xe0, 0xcf, 0xb6, 0x9c, 0x3e, 0x7d, 0x05, 0x20, 0x46, 0xd8, 0x98, 0x9a, 0x9c, 0x97}` in little-endian byte order);
        * *initseq* (`string | table`): 128-bit integer written as a `string` following the regex pattern `0[xX][0-9a-fA-F]{1,32}` (e.g.: `0x0000000000000001da3e39cb94b95bdb`), or a `table` representing the 128-bit integer as a byte array (e.g.: `{0xdb, 0x5b, 0xb9, 0x94, 0xcb, 0x39, 0x3e, 0xda, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}` in little-endian byte order).
    * *Return* (`void`).

[Back to TOC](#table-of-contents)

## Known limitations

1. The integer generated by [pcg32's next](#next) and [pcg64's next](#next-1) might be truncated due to the type `lua_Integer` used to represent Lua integers being shorter in size than 32-bit or 64-bit, respectively. The only way to get from `lua-pcg` the same exact data generated by the PCG algorithms provided by the authors [https://www.pcg-random.org/](https://www.pcg-random.org/) is to call [pcg32's nextbytes](#nextbytes) and [pcg64's nextbytes](#nextbytes-1) in order to deal with bytes, because bytes are never truncated by `lua-pcg`;

2. During the testing phase of `lua-pcg`, a strange behavior regarding Lua 5.1 and Lua 5.2, including LuaJIT, was observed: when Lua / LuaJIT was built by a C compiler (e.g.: tested with GCC 13) that supports 64-bit integers, these versions of Lua are unable to handle some 64-bit integers. For instance, if you execute the code `lua -e "print(string.format('%19i', 1760088112211497577))"` for PUC-Lua interpreter or `luajit -e "print(string.format('%19i', 1760088112211497577))"` for LuaJIT, the output is `1760088112211497472`, when obviously it should be `1760088112211497577`. On Lua 5.3 and Lua 5.4, the correct value is printed normally. So, this behavior is a Lua 5.1, Lua 5.2 and LuaJIT limitation.

    These 64-bit integers commented above that Lua 5.1, Lua 5.2 and LuaJIT handle incorrectly is often generated by `lua-pcg`. For example, if you run the following code on the conditions above

    ```lua
    local pcg = require('lua-pcg')
    local rng = pcg.pcg64.new('0x979c9a98d84620057d3e9cb6cfe0549b', '0x0000000000000001da3e39cb94b95bdb')

    print(string.format('%19i', rng:next()))
    ```

    it prints `1760088112211497472`, when it should print the correct value `1760088112211497577`. However, as explained earlier, it is Lua 5.1, Lua 5.2 and LuaJIT limitation, not a `lua-pcg` bug. Internally, the `pcg64` state holds the correct value, but Lua 5.1, Lua 5.2 and LuaJIT are unable to handle it. The only way to always get the correct data is by calling [pcg64's nextbytes](#nextbytes-1), because Lua / LuaJIT is able to treat bytes in the proper manner. The situation described above happens quite often for 64-bit values. So, if you want accuracy regarding the expected output, you shall use [pcg64's nextbytes](#nextbytes-1). On Lua 5.3 and Lua 5.4, this behavior was not observed.

3. A similar situation explained on (2) may occur on Lua 5.1, Lua 5.2 and LuaJIT on 16-bit operating systems with the function [pcg32's next](#next) to handle 32-bit values. However, I don't have access to such a system to reproduce it. Moreover, the amount of people using 16-bit Lua nowadays most likely is not representative, and the chance of them getting hit by this Lua bug is remote.

## Change log

* v0.0.1: Initial release.