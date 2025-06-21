/*
** The MIT License (MIT)
** 
** Copyright (c) 2025 luau-project [https://github.com/luau-project/lua-pcg](https://github.com/luau-project/lua-pcg)
** 
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
*/

#ifndef LUA_PCG_H
#define LUA_PCG_H

#include <lua.h>

#define LUA_PCG_VERSION "0.0.1"

/*
** DISCLAIMER:
** 
** Since this library works
** on the bit level to emulate
** integer arithmetic, it is
** critical to have a stable
** memory allocator.
** 
** During the testing phase,
** especially with LuaJIT 
** compiled with MinGW-w64 on Windows,
** issues related to memory
** allocation (probably due alignment)
** were detected, and were also solved
** by defining -DLUAJIT_USE_SYSMALLOC
** at LuaJIT building phase, even though
** there is no certainty.
** 
** So, for these reasons,
** the ability to use Lua's own
** allocator was removed
** as the standard behavior.
** However, the chance to use
** Lua's own allocator
** is given through the
** following macro:
** 
** #define LUA_PCG_USE_LUA_ALLOC
** 
** On the other hand,
** if the library fails
** randomly by activating this setting,
** then you are on your own.
*/

/*
** The detection for integer emulation
** is provided automatically by
** this library.
** 
** Mostly targeted to test on C89,
** emulation can also be forced
** as indicated below.
*/

/*
** Do you want to force
** 64-bit integer emulation
** on this library?
** 
** #define LUA_PCG_FORCE_U64_EMULATED
** 
** Note: forcing 64-bit emulation
**       also forces 128-bit emulation.
** 
*/

/*
** Do you want to force
** 128-bit integer emulation
** on this library?
** 
** #define LUA_PCG_FORCE_U128_EMULATED
*/

#ifndef LUA_PCG_EXPORT
#ifdef LUA_PCG_BUILD_STATIC
#define LUA_PCG_EXPORT
#else
#ifdef LUA_PCG_BUILD_SHARED
#if defined(_WIN32)
#if defined(__GNUC__) || defined(__MINGW32__)
#define LUA_PCG_EXPORT __attribute__((dllexport))
#else
#define LUA_PCG_EXPORT __declspec(dllexport)
#endif
#else
#define LUA_PCG_EXPORT __attribute__((visibility("default")))
#endif
#else
#if defined(_WIN32)
#if defined(__GNUC__) || defined(__MINGW32__)
#define LUA_PCG_EXPORT __attribute__((dllimport))
#else
#define LUA_PCG_EXPORT __declspec(dllimport)
#endif
#else
#define LUA_PCG_EXPORT
#endif
#endif
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

LUA_PCG_EXPORT int luaopen_pcg(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif