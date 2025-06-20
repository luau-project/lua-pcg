package = "lua-pcg"
local raw_version = "dev"
version = raw_version .. "-1"

source = {
   url = "git+https://github.com/luau-project/lua-pcg"
}

description = {
   homepage = "https://github.com/luau-project/lua-pcg",
   license = "MIT",
   summary = [[PCG random number generators for Lua]],
   detailed = [=[PCG is a family of simple fast space-efficient statistically good algorithms for random number generation.

Visit the repository for more information.]=]
}

dependencies = {
   "lua >= 5.1"
}

build = {
   type = "builtin",
   modules = {
      ["lua-pcg"] = {
         sources = { "src/lua-pcg.c" },
         defines = { "NDEBUG", "_NDEBUG", "LUA_PCG_BUILD_SHARED" },
         incdirs = { "src" }
      }
   }
}