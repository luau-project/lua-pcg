# Documentation

Welcome to `lua-pcg` documentation! Below, we discuss the process to build `lua-pcg` directly from the command line on Windows, or terminal on Unix systems.

> [!TIP]
> 
> If you are interested to see `lua-pcg` working on DOS, visit this fun fact [here](./DOSBox.md)!

## Table of Contents

* [Prerequisites](#prerequisites)
* [Alternative installation methods](#alternative-installation-methods)
    * [Windows](#windows)
        * [Microsoft Visual C/C++ - MSVC](#microsoft-visual-cc---msvc)
        * [MinGW / MinGW-w64](#mingw--mingw-w64)
    * [Linux](#linux)
    * [macOS](#macos)

## Prerequisites

First, make sure you have access to:

* A working installation of Lua (5.1 or newer);
* The same C toolchain used to build Lua. This toolchain is assumed to be on your system environment `PATH` variable;
* The free and open source distributed version control system `git` (see [https://git-scm.com](https://git-scm.com)). `git` is also assumed to be on your system environment `PATH` variable.

In the following, we describe the building process and installation method split by different operating systems.

## Alternative installation methods

Here, we teach you how to build `lua-pcg` out of LuaRocks.

### Windows

On Windows, we split the guide by toolchain.

#### Microsoft Visual C/C++ - MSVC

1. Open the same Visual Studio command prompt that you used to build Lua
2. Change directory to the temporary directory, remove possibly old directories used earlier, and clone `lua-pcg` from the latest tag:

    ```batch
    cd "%TMP%"
    IF EXISTS "lua-pcg\" RMDIR /S /Q "lua-pcg\"
    git clone --branch=v0.0.1 https://github.com/luau-project/lua-pcg
    cd lua-pcg
    ```

3. Set a variable pointing to the directory containing Lua header files (`lua.h`, `luaconf.h`, `lauxlib.h`, `lualib.h` and `lua.hpp`). For instance, if the full path of `lua.h` is `C:\path to lua\include\lua.h`, then set

    ```batch
    SET LUA_INCDIR=C:\path to lua\include
    ```

4. Set another variable containing the full path for the import library of Lua. Inside Lua directory, it is usually a file named in one of the following forms: `lua.lib`, `lua5.4.lib`, `lua54.lib` (or `luaXY.lib`, `luaX.Y.lib` where `X` is the major version of Lua and `Y` the minor version). For instance, if the import library is located at `C:\path to lua\lib\lua.lib`, then set

    ```batch
    SET LUA_LIB=C:\path to lua\lib\lua.lib
    ```

5. Compile the source code

    ```batch
    cl /c /O2 /W3 /MD "/I%LUA_INCDIR%" /Isrc /DNDEBUG /D_NDEBUG /DLUA_PCG_BUILD_SHARED /Fosrc\lua-pcg.obj src\lua-pcg.c
    ```

6. Build `lua-pcg` shared library

    ```batch
    link /DLL /DEF:lua-pcg.def /OUT:lua-pcg.dll src\lua-pcg.obj "%LUA_LIB%"
    ```

7. *(Optional)* Test the library:
    * Test the class `pcg32`: `lua test\test32.lua`
    * Test the class `pcg64`: `lua test\test64.lua`

8. Now, place the file `lua-pcg.dll` in the same folder as your Lua interpreter (`lua.exe`).

#### MinGW / MinGW-w64

1. Open a vanilla command prompt (`cmd`)
2. Change directory to the temporary directory, remove possibly old directories used earlier, and clone `lua-pcg` from the latest tag:

    ```batch
    cd "%TMP%"
    IF EXISTS "lua-pcg\" RMDIR /S /Q "lua-pcg\"
    git clone --branch=v0.0.1 https://github.com/luau-project/lua-pcg
    cd lua-pcg
    ```

3. Set a variable pointing to the directory containing Lua header files (`lua.h`, `luaconf.h`, `lauxlib.h`, `lualib.h` and `lua.hpp`). For instance, if the full path of `lua.h` is `C:\path to lua\include\lua.h`, then set

    ```batch
    SET LUA_INCDIR=C:\path to lua\include
    ```

4. Set another variable containing the full path for the DLL of Lua. Inside Lua directory, it is usually a file named in one of the following forms: `lua.dll`, `lua5.4.dll`, `lua54.dll` (or `luaXY.dll`, `luaX.Y.dll` where `X` is the major version of Lua and `Y` the minor version). For instance, if the DLL is located at `C:\path to lua\bin\lua.dll`, then set

    ```batch
    SET LUA_DLL=C:\path to lua\bin\lua.dll
    ```

5. Compile the source code

    ```batch
    gcc -c -O2 -Wall "-I%LUA_INCDIR%" -Isrc -DNDEBUG -D_NDEBUG -DLUA_PCG_BUILD_SHARED -o src\lua-pcg.o src\lua-pcg.c
    ```

6. Build `lua-pcg` shared library

    ```batch
    gcc -shared -o lua-pcg.dll src\lua-pcg.o "%LUA_DLL%" -lm
    ```

7. *(Optional)* Test the library:
    * Test the class `pcg32`: `lua test\test32.lua`
    * Test the class `pcg64`: `lua test\test64.lua`

8. Now, place the file `lua-pcg.dll` in the same folder as your Lua interpreter (`lua.exe`).

### Linux

1. Open a terminal
2. Change directory to the temporary directory `/tmp`, remove possibly old directories used earlier, and clone `lua-pcg` from the latest tag:

    ```bash
    cd /tmp
    rm -rf ./lua-pcg
    git clone --branch=v0.0.1 https://github.com/luau-project/lua-pcg
    cd lua-pcg
    ```

3. Set a variable pointing to the directory containing Lua header files (`lua.h`, `luaconf.h`, `lauxlib.h`, `lualib.h` and `lua.hpp`). For instance, if the full path of `lua.h` is `/usr/local/include/lua.h`, then set

    ```bash
    LUA_INCDIR=/usr/local/include
    ```

4. Compile the source code

    ```bash
    cc -c -fPIC -O2 -Wall "-I${LUA_INCDIR}" -Isrc -DNDEBUG -D_NDEBUG -DLUA_PCG_BUILD_SHARED -o src/lua-pcg.o src/lua-pcg.c
    ```

5. Build `lua-pcg` shared library

    ```bash
    cc -shared -o lua-pcg.so src/lua-pcg.o
    ```

6. *(Optional)* Test the library:
    * Test the class `pcg32`: `lua test/test32.lua`
    * Test the class `pcg64`: `lua test/test64.lua`

7. Now, place the file `lua-pcg.so` in the folder reserved for Lua C modules
    * On a standard Lua installation from the sources, it should be `/usr/local/lib/lua/X.Y` where `X` is the major version of Lua and `Y` the minor version
    * If you installed Lua by the package manager from the distribution:
        * On Debian-derived distributions (e.g.: Ubuntu), it is `/usr/lib/x86_64-linux-gnu/lua/X.Y` for `amd64` architecture, where `X` is the major version of Lua and `Y` the minor version
        * On RedHat distributions (e.g.: Fedora), it is `/usr/lib64/lua/X.Y` for `amd64` architecture, where `X` is the major version of Lua and `Y` the minor version

### macOS

1. Open a terminal
2. Change directory to the temporary directory `/tmp`, remove possibly old directories used earlier, and clone `lua-pcg` from the latest tag:

    ```bash
    cd /tmp
    rm -rf ./lua-pcg
    git clone --branch=v0.0.1 https://github.com/luau-project/lua-pcg
    cd lua-pcg
    ```

3. Set a variable pointing to the directory containing Lua header files (`lua.h`, `luaconf.h`, `lauxlib.h`, `lualib.h` and `lua.hpp`). For instance, if the full path of `lua.h` is `/usr/local/include/lua.h`, then set

    ```bash
    LUA_INCDIR=/usr/local/include
    ```

4. Set a variable pointing to your desired macOS deployment target (change below according to your needs):

    ```bash
    MACOSX_DEPLOYMENT_TARGET=11.0 
    ```

5. Compile the source code

    ```bash
    env "MACOSX_DEPLOYMENT_TARGET=$MACOSX_DEPLOYMENT_TARGET" cc -c -fPIC -O2 -Wall "-I$LUA_INCDIR" -Isrc -DNDEBUG -D_NDEBUG -DLUA_PCG_BUILD_SHARED -o src/lua-pcg.o src/lua-pcg.c
    ```

6. Build `lua-pcg` shared library

    ```bash
    env "MACOSX_DEPLOYMENT_TARGET=$MACOSX_DEPLOYMENT_TARGET" cc -bundle -undefined dynamic_lookup -all_load -o lua-pcg.so src/lua-pcg.o
    ```

7. *(Optional)* Test the library:
    * Test the class `pcg32`: `lua test/test32.lua`
    * Test the class `pcg64`: `lua test/test64.lua`

8. Now, place the file `lua-pcg.so` in the folder reserved for Lua C modules
    * On a standard Lua installation from the sources, it should be `/usr/local/lib/lua/X.Y` where `X` is the major version of Lua and `Y` the minor version

---

[Back to TOC](#table-of-contents) | [Back to home](../)