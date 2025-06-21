# Build, run and test lua-pcg on DOS using a Linux host

`lua-pcg` is written in C89. In order to ensure the ability of `lua-pcg` to build, run and pass tests on old systems with limited resources, we are going to setup a combination of Lua 5.1.5 and `lua-pcg` on DOSBox ([https://www.dosbox.com/](https://www.dosbox.com/)), which is an x86 emulator with DOS.

> [!TIP]
> 
> For instance, MS-DOS, a 16-bit operating system, is the precursor of Windows. Through emulation, DOSBox offers its own shell that is quite similar to the current `cmd` (command prompt) on Windows. However, the DOSBox shell lacks many `cmd` features from the modern Windows versions (in my guesses, the differences are most likely due to DOS limitations from that era).

> [!IMPORTANT]
> 
> If you don't feel comfortable to install random stuff on your computer from random people in the internet, follow this tutorial on a virtual machine. In my case, I used a separate partition on Arch Linux (x64) to write this tutorial.

## Table of Contents

* [Prerequisites](#prerequisites)
* [Setup the host environment](#setup-the-host-environment)
    * [Create the layout of directories](#create-the-layout-of-directories)
    * [Patch Lua 5.1.5 source code](#patch-lua-515-source-code)
        * [Patch luaconf.h](#patch-luaconfh)
        * [Patch lua.c to bundle lua-pcg together with Lua](#patch-luac-to-bundle-lua-pcg-together-with-lua)
    * [Generate the build instructions](#generate-the-build-instructions)
        * [Merge lua-pcg source code with Lua 5.1.5 source code](#merge-lua-pcg-source-code-with-lua-515-source-code)
        * [Generate the batch file responsible for the build](#generate-the-batch-file-responsible-for-the-build)
    * [Configure DOSBox on the Linux host](#configure-dosbox-on-the-linux-host)
        * [Configure the layout of the keyboard](#configure-the-layout-of-the-keyboard)
        * [Configure the drive to be mounted on DOSBox](#configure-the-drive-to-be-mounted-on-dosbox)
        * [Configure the directories to be added to system environment PATH variable on DOSBox](#configure-the-directories-to-be-added-to-system-environment-path-variable-on-dosbox)
        * [Review of the configuration file](#review-of-the-configuration-file)
* [Inside DOSBox](#inside-dosbox)
    * [Install Turbo C++](#install-turbo-c)
    * [Build Lua 5.1.5 and lua-pcg](#build-lua-515-and-lua-pcg)
    * [Run simple tests on the Lua interpreter just built](#run-simple-tests-on-the-lua-interpreter-just-built)
    * [Run lua-pcg test suite](#run-lua-pcg-test-suite)

## Prerequisites

This tutorial requires a few tools to be installed:

* `wget` (or `curl`)
* `unzip`
* `tar`
* `git`
* `dosbox`

To install these tools, depending on your operating system, you have to use the following commands on your terminal:

* Debian-derived distros (e.g.: Ubuntu, Mint):

    ```bash
    sudo apt install wget unzip tar git dosbox
    ```

* RedHat distros (e.g.: Fedora):

    ```bash
    sudo dnf install wget unzip tar git dosbox
    ```

* Arch Linux:

    ```bash
    sudo pacman -S install wget unzip tar git dosbox
    ```

For other distributions not listed above, search on the internet how to install them.

## Setup the host environment

On this section, we are going to:

* Create the layout of directories;
* Then we patch Lua source code to build on DOS;
* And, finally, we generate the build instructions on our Linux host to build Lua bundled with `lua-pcg` inside DOSBox.

### Create the layout of directories

For this guide, we'll create a separate directory on your home directory.

1. Not surprisingly, we are going to create it at `$HOME/dosbox`:

    ```bash
    cd $HOME
    mkdir -p dosbox
    cd dosbox
    ```

2. Download a well known C compiler (C89 compliant) old enough to work on DOS. For such task, we'll download Borland Turbo C++ 1.01 (a C compiler from 1990) directly from embarcadero museum (*embarcadero bought Borland many years ago*):
[https://altd.embarcadero.com/download/museum/tcpp101.zip](https://altd.embarcadero.com/download/museum/tcpp101.zip)

    * Using `wget`:

        ```bash
        cd $HOME/dosbox
        wget https://altd.embarcadero.com/download/museum/tcpp101.zip
        ```

    * Or, in case of `curl`:

        ```bash
        cd $HOME/dosbox
        curl -L -O https://altd.embarcadero.com/download/museum/tcpp101.zip
        ```

3. Create a subdirectory named `tc-src` inside of `$HOME/dosbox`, and extract Turbo C++ inside it:

    ```bash
    cd $HOME/dosbox
    mkdir -p tc-src
    cd tc-src
    unzip ../tcpp101.zip
    ```

4. Download Lua 5.1.5 source code:

    * Using `wget`:

        ```bash
        cd $HOME/dosbox
        wget https://lua.org/ftp/lua-5.1.5.tar.gz
        ```

    * Or, in case of `curl`:

        ```bash
        cd $HOME/dosbox
        curl -L -O https://lua.org/ftp/lua-5.1.5.tar.gz
        ```

5. Extract Lua source code inside `$HOME/dosbox`, and rename the directory created by the extraction of the tarball to `lua` (**tip**: here, we rename `lua-5.1.5` to `lua`, because DOS has a limitation allowing filenames and directories at most 8 characters long):

    ```bash
    cd $HOME/dosbox
    tar -xf lua-5.1.5.tar.gz
    mv lua-5.1.5 lua
    ```

6. Change directory back to `$HOME/dosbox` and use `git` to clone `lua-pcg` from the latest available tag:

    ```bash
    cd $HOME/dosbox
    git clone --branch=v0.0.1 --single-branch https://github.com/luau-project/lua-pcg
    ```

7. Create a subdirectory `bld-lua` inside `$HOME/dosbox` (possibly removing old directories from previous runs) to store object files resulting from the compilation of each C file:

    ```bash
    cd $HOME/dosbox
    rm -rf bld-lua
    mkdir -p bld-lua
    ```

8. After all these steps, verify that you have the following directory tree

    ```bash
    $HOME/dosbox
            |
            | --- bld-lua
            |
            | --- lua
            |
            | --- lua-pcg
            |
            | --- tc-src 
    ```

    such that `bld-lua` is empty, `lua` holds Lua 5.1.5 source code, `lua-pcg` holds the source code of this library, and `tc-src` holds a bunch of files from Turbo C++.

### Patch Lua 5.1.5 source code

Lua 5.1.5 builds almost unmodified on DOS. If you just want to build Lua (without `lua-pcg`), only [Patch luaconf.h](#patch-luaconfh) is needed. If you want to test `lua-pcg` on DOS, you also have to [Patch lua.c to bundle lua-pcg together with Lua](#patch-luac-to-bundle-lua-pcg-together-with-lua), because DOS is unable to load shared libraries dynamically (DOS lacks `LoadLibrary` -- this API is available only on Windows NT). Thus, you have to merge `lua-pcg` source code with Lua 5.1.5 to force the Lua interpreter to load `lua-pcg` on startup.

#### Patch luaconf.h

If you want to patch manually, open the file `$HOME/dosbox/lua/src/luaconf.h` on your text editor

1. On line 83, replace `#if defined(_WIN32)` by `#if !defined(_WIN32)`
2. On line 113, replace `#if defined(_WIN32)` by `#if !defined(_WIN32)` too.

Alternatively, a quick manner to apply these changes is:

1. Paste the following content to a file named `$HOME/dosbox/lua/luaconf-5.1.5.patch`:

    ```patch
    diff -Naur lua-5.1.5/src/luaconf.h lua/src/luaconf.h
    --- lua-5.1.5/src/luaconf.h	2008-02-11 13:25:08.000000000 -0300
    +++ lua/src/luaconf.h	2025-06-15 12:19:01.460571700 -0300
    @@ -80,7 +80,7 @@
    ** hierarchy or if you want to install your libraries in
    ** non-conventional directories.
    */
    -#if defined(_WIN32)
    +#if !defined(_WIN32)
    /*
    ** In Windows, any exclamation mark ('!') in the path is replaced by the
    ** path of the directory of the executable file of the current process.
    @@ -110,7 +110,7 @@
    ** CHANGE it if your machine does not use "/" as the directory separator
    ** and is not Windows. (On Windows Lua automatically uses "\".)
    */
    -#if defined(_WIN32)
    +#if !defined(_WIN32)
    #define LUA_DIRSEP	"\\"
    #else
    #define LUA_DIRSEP	"/"
    ```

2. Change directory to `$HOME/dosbox/lua` and use `git` to apply the patch above:

    ```bash
    cd $HOME/dosbox/lua
    git apply luaconf-5.1.5.patch
    ```

#### Patch lua.c to bundle lua-pcg together with Lua

The goal of patching `lua.c` is to force the Lua interpreter to load `lua-pcg` on startup, because as explained earlier, being an ancient system, DOS is unable to load shared libraries dynamically through `LoadLibrary` API. In case you don't want `lua-pcg` on DOS, don't apply this patch.

Manual changes:

1. On line 19, add `#include "lua-pcg.h"` below `#include "lualib.h"`
2. Around line 349, below the line `luaL_openlibs(L);  /* open libraries */`, add

    ```c
    lua_pushcfunction(L, luaopen_pcg);
    lua_call(L, 0, 1);
    lua_setglobal(L, "pcg");
    ```

Alternatively, apply these changes using a patch file as follows:

1. Paste the following content on a file named `$HOME/dosbox/lua/lua.c-5.1.5-lua-pcg.patch`:

    ```patch
    diff -Naur lua-5.1.5/src/lua.c lua/src/lua.c
    --- lua-5.1.5/src/lua.c	2007-12-28 12:32:23.000000000 -0300
    +++ lua/src/lua.c	2025-06-15 19:26:39.385371700 -0300
    @@ -16,6 +16,7 @@

    #include "lauxlib.h"
    #include "lualib.h"
    +#include "lua-pcg.h"



    @@ -346,6 +347,9 @@
    if (argv[0] && argv[0][0]) progname = argv[0];
    lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
    luaL_openlibs(L);  /* open libraries */
    +  lua_pushcfunction(L, luaopen_pcg);
    +  lua_call(L, 0, 1);
    +  lua_setglobal(L, "pcg");
    lua_gc(L, LUA_GCRESTART, 0);
    s->status = handle_luainit(L);
    if (s->status != 0) return 0;
    ```

2. Change directory to `$HOME/dosbox/lua` and use `git` to apply the patch above:

    ```bash
    cd $HOME/dosbox/lua
    git apply lua.c-5.1.5-lua-pcg.patch
    ```

### Generate the build instructions

#### Merge lua-pcg source code with Lua 5.1.5 source code

In order to bundle our library together with Lua, we copy `lua-pcg` files to the directory of Lua:

```bash
cp $HOME/dosbox/lua-pcg/src/* $HOME/dosbox/lua/src
```

If you don't want to build Lua bundled with `lua-pcg`, skip this merge step.

#### Generate the batch file responsible for the build

On this step, rather than writing a build command ourselves by hand on DOSBox shell, we are going to pack the whole build instructions on a single batch file (named `$HOME/dosbox/lua/lua-bld.bat`) on our Linux host.

1. Write the following content to a file named `$HOME/dosbox/lua/generate.sh`

    ```bash
    # remove possibly old batch file
    rm -f $HOME/dosbox/lua/lua-bld.bat

    # write a 'change directory' to Lua source code dir
    echo "cd C:\\LUA\\SRC" >> $HOME/dosbox/lua/lua-bld.bat

    # for each Lua source file
    # starting with 'l' containing .c extension
    for lua_file in $HOME/dosbox/lua/src/l*.c;
    do
        # if the file is NOT 'luac.c'
        if ! [ "$(basename $lua_file)" = "luac.c" ];
        then
            # write a 'compile-only' instruction for such C file
            #
            # TCC (Turbo C++ Compiler)
            # -c: compile-only switch
            # -G: optimize for speed
            # -w-: disable any warnings
            # -mh: compile for the Huge memory model (this switch is equivalent to -D__HUGE__). In practice, it uses ptrdiff_t as long (32-bit memory)
            # -nC:\\BLD-LUA: place object files (.obj) on C:\BLD-LUA
            # -I.: include the current directory as include directory
            # -DLUA_ANSI: build Lua 5.1.5 constrained to use C89
            # -DLUA_PCG_BUILD_STATIC: build lua-pcg as a static library
            # $(basename $lua_file): expands to the current file name
            echo "TCC -c -G -w- -mh -nC:\\BLD-LUA -I. -DLUA_ANSI -DLUA_PCG_BUILD_STATIC $(basename $lua_file)" >> $HOME/dosbox/lua/lua-bld.bat;
        fi
    done

    # write a 'change directory' to the base drive
    echo "cd C:\\" >> $HOME/dosbox/lua/lua-bld.bat

    # write a command to the batch file
    # to link object files on C:\BLD-LUA with .OBJ extension
    # generating an executable (-elua) named lua
    echo "TCC -G -w- -mh -elua C:\\BLD-LUA\\*.OBJ" >> $HOME/dosbox/lua/lua-bld.bat
    ```

2. Execute the file `generate.sh` from the previous step to create the batch file responsible for the build:

    ```bash
    sh $HOME/dosbox/lua/generate.sh
    ```

3. Verify that the file `$HOME/dosbox/lua/lua-bld.bat` was created with a bunch of TCC lines as indicated above.

### Configure DOSBox on the Linux host

Now, we are ready to configure DOSBox for the directory layout created on the previous section.

On my operating system, DOSBox stores its configuration inside the directory `$HOME/.dosbox$` on a file with `.conf` extension. At the moment, using DOSBox 0.74-3, the full path to the configuration file is `$HOME/.dosbox/dosbox-0.74-3.conf`.

#### Configure the layout of the keyboard

On Windows, the keyboard layout detection on DOSBox works nicely. However, on Linux, based on my personal experience, DOSBox was unable to detect the correct setting. You have to launch DOSBox shell and type a few words to test whether the default setting works for you or not.

For instance, the default setting `keyboardlayout=auto` didn't work for me, then I had to set it explicitly `keyboardlayout=br` to make things work.

In case the default setting does not work for you, to solve this, open DOSBox configuration file on your text editor, scroll down to the `[dos]` section, and set the field `keyboardlayout` according to the page [https://www.dosbox.com/wiki/KEYB](https://www.dosbox.com/wiki/KEYB).

> [!NOTE]
> 
> On my personal tests, on a Debian 32-bits virtual machine without graphical user interface, I was unable to configure the keyboard layout even setting it explicitly in the configuration file.

#### Configure the drive to be mounted on DOSBox

This section is optional, but since you may have to execute the command many times, I like to have it added on my configuration file so that I don't need to keep repeating it over and over.

Again, open DOSBox configuration file on your text editor. Then scroll down to the `[autoexec]` section. There, add a line to automatically mount a `C:` drive binding it to the base directory `$HOME/dosbox`, then another line to change directory to the newly mounted `C:` drive:

```batch
mount C /home/luau-project/dosbox
C:
```

> [!NOTE]
> 
> Replace `luau-project` by your username.

By default, everytime, the DOSBox shell launchs on its own `Z:` drive without any directory mounted. Thus, the mount command above binds `C:\` (root C drive on DOS) to your `$HOME/dosbox` directory, so you don't need to repeat it again.

The last line `C:` tells DOSBox to change directory (DOSBox starts on `Z:\`) to the `C:` drive mounted on the previous step.

#### Configure the directories to be added to system environment PATH variable on DOSBox

This section is also optional, but I also recommend it. As said, we are going to install Turbo C++ to build Lua 5.1.5 and `lua-pcg` source code. It is a wise choice to add the directory of Turbo C++ binaries to the system environment PATH variable for the system to find it.

Open DOSBox configuration file, scroll down to the `[autoexec]` section, and add the line

```batch
path C:\TC\BIN;C:\
```

Since we are going to install Turbo C++ on a folder named `C:\TC`, the binary directory which contains the C compiler and linker (`C:\TC\BIN`) will be available without the need to write its entire path on the DOSBox shell. We are also going to place the Lua interpreter on `C:\` directory. Then `lua` will also be on command line without specifying the full path.

#### Review of the configuration file

If you followed all the instructions, then scrolling down to the end, the `[autoexec]` section on your configuration file should look like

```ini
[autoexec]
# Lines in this section will be run at startup.
# You can put your MOUNT lines here.
mount C /home/luau-project/dosbox
C:
path C:\TC\BIN;C:\
```

Again, you shall replace `luau-project` by your username.

## Inside DOSBox

If you got this far, you are indeed a brave adventurer passionate about exploring Lua on such ancient operating systems like the one DOSBox offers us (and potentially trying `lua-pcg` there).

So, go ahead and launch DOSBox.

> [!TIP]
> 
> In the same spirit as Windows, the filesystem provided by DOSBox is case insensitive on paths. So, the paths `C:\TC-SRC` and `c:\tc-src` are exactly the same within the DOSBox environment.

You should see this startup screen

![001-dosbox-startup](https://github.com/user-attachments/assets/b88ff0ee-6a48-47e5-b3d7-d16982abc070)

### Install Turbo C++

1. Once DOSBox shell loads and performs out instructions on `[autoexec]` above, change directory to `tc-src` (like a Unix shell, DOSBox shell also accepts tab completions). The directory `tc-src` contains the zip files and bootstrap applications to install Turbo C++ (use the autocomplete while typing).

    ```batch
    cd C:\TC-SRC
    ```

2. Execute the `install.exe` application:

    ```batch
    INSTALL.EXE
    ```

    ![002-cd-tc-src-invoke-install](https://github.com/user-attachments/assets/23b61758-4975-45f6-aa81-dc0d19cf0c39)

4. Once it loaded, you should see this screen

    ![003-install-initial-screen](https://github.com/user-attachments/assets/5e028cd2-bbeb-4298-bf3f-5266d93a2702)

    Press enter.

6. When asked for the SOURCE drive to use, type `C`:

    ![004-enter-drive-c](https://github.com/user-attachments/assets/310aada3-aa88-488d-bd9d-b395745e6584)

    Press enter.

8. The installation manager of Turbo C++ will ask where the source files can be found (directory containing zip files). In case it is not there, type `\TC-SRC`:

    ![005-tc-src-directory](https://github.com/user-attachments/assets/ca9ddd3e-f995-420a-9fc5-3639a6d68b80)

    Press enter.

10. The next screen asks for you to define several paths:

    ![006-initial-turbo-cpp-install-directory](https://github.com/user-attachments/assets/178b780d-c0a3-491b-bff8-abde650ef12e)

    We are going to accept the defaults. Then hit the `arrow down` on your keyboard to select `Start Installation` option.

> [!IMPORTANT]
> 
> **DO NOT** change defaults. Our `lua-bld.bat` depends on these paths.

12. Once the `Start Installation` option is selected, hit enter:

    ![007-scroll-down-start-installation](https://github.com/user-attachments/assets/b13c656c-bea5-48b8-983a-90fe89bd8223)

14. On a successfull installation, we see this screen:

    ![008-installation-success](https://github.com/user-attachments/assets/ca899ad4-28fb-4029-9d44-7b32ca47e774)

    Press enter.

16. On the next screen, we can see this:

    ![009-after-pressing-enter](https://github.com/user-attachments/assets/b6cd4d16-d21e-497a-81b5-545c5ef52fb2)

    Press enter.

18. At this moment, everything should have been installed. Run the command

    ```batch
    tcc
    ```

    to test Turbo C++ compiler:

    ![010-test-turbo-c-compiler](https://github.com/user-attachments/assets/e06ce61a-ba10-46e7-a742-dd31cc17a006)

20. Check Turbo C++ compiler output:

    ![011-turbo-c-compiler-output](https://github.com/user-attachments/assets/c996b162-d434-44d5-a32b-2d74c6a19de2)

22. It is a good idea to also test Turbo Link (the linker):

    ```batch
    tlink
    ```

    You should see this output:

    ![012-turbo-link-output](https://github.com/user-attachments/assets/576338b6-4a85-4628-a0af-38aa4f628d2d)

### Build Lua 5.1.5 and lua-pcg

Now, we are going to build the Lua interpreter `lua.exe`.

1. Just call the bat file:

    ```batch
    C:\LUA\LUA-BLD.BAT
    ```

    ![013-call-lua-bld-bat](https://github.com/user-attachments/assets/99e852bb-ea1a-4d6b-913d-e780313cf86d)

3. If everything was successfull, you should see this screen:

    ![014-end-of-build](https://github.com/user-attachments/assets/ea79f834-1d16-4b97-933f-2f11303c9d9c)

### Run simple tests on the Lua interpreter just built

Now, the Lua interpreter has been built merged with `lua-pcg`. Run some basic test script to see the interpreter working.

1. Check Lua version:

    ```batch
    lua -v
    ```

    ![015-lua-version](https://github.com/user-attachments/assets/b05a5ffe-9283-4f16-97a0-7bb247327da9)

3. Run some code:

    ```batch
    lua -e "for i = 1, 10 do print(i) end"
    ```

    ![016-lua-for](https://github.com/user-attachments/assets/879e1351-9adf-4986-b8ec-0b0214f00906)

5. Check that `lua-pcg` was also bundled with Lua:

    ```batch
    lua -e "print(pcg ~= nil and pcg.pcg32 ~= nil and pcg.pcg64 ~= nil)"
    ```

    This should print `true` like the image below

    ![017-lua-pcg-was-bundled](https://github.com/user-attachments/assets/c0989d4d-ffb8-4c82-91b7-41c519565b4e)

### Run lua-pcg test suite

> [!WARNING]
> 
> On my machine, **I did it only once**, the whole test suite takes more than eight (8) hours to run completely. Around one (1) hour in the `test32.lua` file to test `pcg32` class, and the rest on `test64.lua` to test `pcg64` class.

Since you have been properly warned that testing takes too long, go ahead if you want.

1. Change directory to `lua-pcg` directory:

    ```batch
    cd C:\LUA-PCG
    ```

    ![018-cd-lua-pcg](https://github.com/user-attachments/assets/707b2d20-ce1d-4060-bcb4-944563afc4c3)

3. Run `test32.lua` to test `pcg32` class:

    ```batch
    lua TESTS\TEST32.LUA
    ```

    ![019-run-test32](https://github.com/user-attachments/assets/60b1bc75-ebcc-47c0-81cc-1a7a5e488a11)

    Now, it is going to take some decent amount of time (around 1h on my computer). If you are patient enough to wait them, at the end of the tests, you can see the following screen:

    ![020-test32-completed](https://github.com/user-attachments/assets/0741e1b7-c881-4832-a2ca-b3c0f47a6810)


5. When it finishes, go ahead and `test64.lua` to test `pcg64` class:

    ```batch
    lua TESTS\TEST64.LUA
    ```

    ![021-run-test64](https://github.com/user-attachments/assets/82d8c195-cf50-4461-965f-5ffd81a6aeed)

    On my computer, it takes 7h or more to finish.

7. In case any of the tests failed, please, open an issue.

---

[Back to TOC](#table-of-contents) | [Back to docs](./README.md) | [Back to home](../)
