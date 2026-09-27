# Build lua-pcg on Unix

This page details how to build and install `lua-pcg` directly from the source code using `cmake`.

> [!TIP]
> 
> * If you are interested to see `lua-pcg` working on DOS, visit this fun fact [here](./DOSBox.md)!
> * In order to build `lua-pcg` manually from the command line or terminal, check the [docs](./README.md).

## Table of Contents

* [Prerequisites](#prerequisites)
* [Build and Install](#build-and-install)

## Prerequisites

* Lua (&ge; 5.1) or LuaJIT must be installed in the system;

* CMake: since `v0.1.0`, it is possible to employ `cmake` to build `lua-pcg` directly from the source code, out of `LuaRocks`. From now on, we are going to assume that `cmake` is installed in the system.

> [!NOTE]
> 
> **Install CMake**: In order to use this method, the `cmake` tool is required.
>
> * On macOS, visit the website [https://cmake.org/](https://cmake.org/), download and install it;
> * On Unix distributions, use the package manager of the system to install it.

## Build and Install

1. Download the latest source code of `lua-pcg`, extract it and open a terminal in the `lua-pcg` directory;

2. Configure `lua-pcg` for the Lua version installed:

    ```bash
    cmake --install-prefix /usr/local -DCMAKE_BUILD_TYPE=Release -B build
    ```

> [!TIP]
> 
> In case multiple Lua versions are installed in the system, use `-DLUA_VERSION=5.1`, ..., `-DLUA_VERSION=5.5` to select the appropriate version for PUC-Lua or `-DLUA_VERSION=luajit` for LuaJIT.

3. Build `lua-pcg`:

    ```bash
    cmake --build build --config Release
    ```

4. Test `lua-pcg`:

    ```bash
    ctest --test-dir build -C Release
    ```

5. Install `lua-pcg` (_super user privileges or **sudo** may be required for this step to work_):

    ```bash
    cmake --install build --config Release
    ```

---

[Back to TOC](#table-of-contents) | [Back to home](../README.md)