# Build, run and test lua-pcg on DOS using a Windows host

`lua-pcg` is written in C89. In order to ensure the ability of `lua-pcg` to build, run and pass tests on old systems with limited resources, we are going to setup a combination of Lua 5.1.5 and `lua-pcg` on DOSBox ([https://www.dosbox.com/](https://www.dosbox.com/)), which is an x86 emulator with DOS.

> [!TIP]
> 
> For instance, MS-DOS, a 16-bit operating system, is the precursor of Windows. Through emulation, DOSBox offers its own shell that is quite similar to the current `cmd` (command prompt) on Windows. However, the DOSBox shell lacks many `cmd` features from the modern Windows versions (in my guesses, the differences are most likely due to DOS limitations from that era).

> [!IMPORTANT]
> 
> If you don't feel comfortable to install random stuff on your computer from random people in the internet, follow this tutorial on a virtual machine.

## Table of Contents

* [Prerequisites](#prerequisites)
* [Setup the host environment](#setup-the-host-environment)

## Prerequisites

This tutorial requires a few tools to be installed:

* `7z`: download from [https://www.7-zip.org/download.html](https://www.7-zip.org/download.html), then install it;
* `git`: download from [https://git-scm.com/downloads](https://git-scm.com/downloads), then install it;
* `dosbox`: download from [https://www.dosbox.com/download.php?main=1](https://www.dosbox.com/download.php?main=1), then install it.

> [!NOTE]
> 
> Throughout this guide, we'll be using a command prompt (`cmd`) to perform certain tasks. So, open a `cmd` and leave it open.

It is a wise choice to test on `cmd` that all the required tools were installed. Run these commands to check the installation of each program:

```cmd
git --version
SET "PATH=%ProgramFiles%\7-zip;%PATH%"
7z
```

> [!TIP]
> 
> The above instruction assumes a standard `7z` installation on `Program Files`. If the command above did not work because you installed `7z` on a custom directory, then you have to add the directory containing `7z.exe` to system environment PATH variable:
> 
> ```cmd
> SET "PATH=C:\my\path\to\7z-folder;%PATH%"
> ```

## Setup the host environment

On this section, we are going to:

* Create the layout of directories;
* Then we patch Lua source code to build on DOS;
* And, finally, we generate the build instructions on our Windows host to build Lua bundled with `lua-pcg` inside DOSBox.

### Create the layout of directories

For this tutorial, in order to avoid any potential issues, we'll create a separate directory on your system drive directory.

> [!TIP]
> 
> Below, we make use of the standard `%SystemDrive%` environment variable. It expands to the drive where your system is installed (e.g.: `C:` for a default installation).

1. Change directory to `%SystemDrive%`, then create a directory named `dosbox` if it does not exist yet. Then change directory to the newly created directory:

    ```cmd
    cd %SystemDrive%
    IF NOT EXIST ".\dosbox\" MKDIR ".\dosbox"
    cd dosbox
    ```

2. Download a well known C compiler (C89 compliant) old enough to work on DOS. For such task, we'll download Borland Turbo C++ 1.01 (a C compiler from 1990) directly from embarcadero museum (*embarcadero bought Borland many years ago*) by clicking the link [https://altd.embarcadero.com/download/museum/tcpp101.zip](https://altd.embarcadero.com/download/museum/tcpp101.zip). Then, we are going to copy this `tcpp101.zip` file from your `Downloads` folder to the created directory:

    ```cmd
    IF NOT EXIST %SystemDrive%\dosbox\tcpp101.zip COPY "%userprofile%\Downloads\tcpp101.zip" %SystemDrive%\dosbox\tcpp101.zip
    ```

3. Create a subdirectory named `tc-src` inside of `%SystemDrive%\dosbox`, and extract Turbo C++ inside it:

    ```cmd
    cd %SystemDrive%\dosbox
    IF NOT EXIST ".\tc-src\" MKDIR ".\tc-src"
    cd tc-src
    7z x ..\tcpp101.zip
    ```

4. Download Lua 5.1.5 source code from Lua website (click [https://lua.org/ftp/lua-5.1.5.tar.gz](https://lua.org/ftp/lua-5.1.5.tar.gz)) to your `Downloads` folder. Again, we are going to copy the downloaded source code to the DOSBox directory:

    ```cmd
    cd %SystemDrive%\dosbox
    IF NOT EXIST %SystemDrive%\dosbox\lua-5.1.5.tar.gz COPY "%userprofile%\Downloads\lua-5.1.5.tar.gz" %SystemDrive%\dosbox\lua-5.1.5.tar.gz
    ```

5. Extract Lua source code inside `%SystemDrive%\dosbox`, and rename the directory created by the extraction of the tarball to `lua` (**tip**: here, we rename `lua-5.1.5` to `lua`, because DOS has a limitation allowing filenames and directories at most 8 characters long):

    ```cmd
    cd %SystemDrive%\dosbox
    7z x lua-5.1.5.tar.gz
    7z x lua-5.1.5.tar
    IF NOT EXIST ".\lua\" move lua-5.1.5 lua
    ```

6. Change directory back to `%SystemDrive%\dosbox` and use `git` to clone `lua-pcg` from the latest available tag:

    ```cmd
    cd %SystemDrive%\dosbox
    git clone --branch=v0.0.1 --single-branch https://github.com/luau-project/lua-pcg
    ```

7. Create a subdirectory `bld-lua` inside `%SystemDrive%\dosbox` (possibly removing old directories from previous runs) to store object files resulting from the compilation of each C file:

    ```cmd
    cd %SystemDrive%\dosbox
    IF EXIST ".\bld-lua\" RMDIR /S /Q bld-lua
    IF NOT EXIST ".\bld-lua\" MKDIR bld-lua
    ```

8. After all these steps, verify that you have the following directory tree

    ```cmd
    %SystemDrive%\dosbox
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

If you want to patch manually, open the file `%SystemDrive%\dosbox\lua\src\luaconf.h` on your `notepad`

1. On line 83, replace `#if defined(_WIN32)` by `#if !defined(_WIN32)`
2. On line 113, replace `#if defined(_WIN32)` by `#if !defined(_WIN32)` too.

Alternatively, a quick manner to apply these changes is:

1. Paste the following content to a file named `%SystemDrive%\dosbox\lua\luaconf-5.1.5.patch`:

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

2. Change directory to `%SystemDrive%\dosbox\lua` and use `git` to apply the patch above:

    ```cmd
    cd %SystemDrive%\dosbox\lua
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

1. Paste the following content on a file named `%SystemDrive%\dosbox\lua\lua.c-5.1.5-lua-pcg.patch`:

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

2. Change directory to `%SystemDrive%\dosbox\lua` and use `git` to apply the patch above:

    ```cmd
    cd %SystemDrive%\dosbox\lua
    git apply lua.c-5.1.5-lua-pcg.patch
    ```

### Generate the build instructions

#### Merge lua-pcg source code with Lua 5.1.5 source code

In order to bundle our library together with Lua, we copy `lua-pcg` files to the directory of Lua:

```cmd
COPY /Y %SystemDrive%\dosbox\lua-pcg\src\* %SystemDrive%\dosbox\lua\src
```

If you don't want to build Lua bundled with `lua-pcg`, skip this merge step.

#### Generate the batch file responsible for the build

On this step, rather than writing a build command ourselves by hand on DOSBox shell, we are going to pack the whole build instructions on a single batch file (named `%SystemDrive%\dosbox\lua\lua-bld.bat`) on our Windows host.

1. Write the following content to a file named `%SystemDrive%\dosbox\lua\generate.bat`

    ```cmd
    @REM remove possibly old batch file
    IF EXIST %SystemDrive%\dosbox\lua\lua-bld.bat DEL %SystemDrive%\dosbox\lua\lua-bld.bat

    @REM write a 'change directory' to Lua source code dir
    echo cd C:\LUA\SRC>> %SystemDrive%\dosbox\lua\lua-bld.bat

    @REM change directory to Lua source code directory
    cd %SystemDrive%\dosbox\lua\src

    @REM for each Lua source file
    @REM starting with 'l' containing .c extension
    FOR %%F IN (l*.c) DO (
        @REM if the file is NOT 'luac.c'
        IF NOT "%%F"=="luac.c" (
            @REM write a 'compile-only' instruction for such C file
            @REM 
            @REM TCC (Turbo C++ Compiler)
            @REM -c: compile-only switch
            @REM -G: optimize for speed
            @REM -w-: disable any warnings
            @REM -mh: compile for the Huge memory model (this switch is equivalent to -D__HUGE__). In practice, it uses ptrdiff_t as long (32-bit memory)
            @REM -nC:\BLD-LUA: place object files (.obj) on C:\BLD-LUA
            @REM -I.: include the current directory as include directory
            @REM -DLUA_ANSI: build Lua 5.1.5 constrained to use C89
            @REM -DLUA_PCG_BUILD_STATIC: build lua-pcg as a static library
            @REM $(basename $lua_file): expands to the current file name
            echo TCC -c -G -w- -mh -nC:\BLD-LUA -I. -DLUA_ANSI -DLUA_PCG_BUILD_STATIC %%F>> %SystemDrive%\dosbox\lua\lua-bld.bat
        )
    )

    @REM write a 'change directory' to the base drive
    echo cd C:\>> %SystemDrive%\dosbox\lua\lua-bld.bat

    @REM write a command to the batch file
    @REM to link object files on C:\BLD-LUA with .OBJ extension
    @REM generating an executable (-elua) named lua
    echo TCC -G -w- -mh -elua C:\BLD-LUA\*.OBJ>> %SystemDrive%\dosbox\lua\lua-bld.bat
    ```

2. Execute the file `generate.bat` from the previous step to create the batch file responsible for the build:

    ```cmd
    %SystemDrive%\dosbox\lua\generate.bat
    ```

3. Verify that the file `%SystemDrive%\dosbox\lua\lua-bld.bat` was created with a bunch of TCC lines as indicated above.

### Configure DOSBox on the Windows host

Now, we are ready to configure DOSBox for the directory layout created on the previous section.

On Windows, assuming a standard DOSBox installation from the Windows installer, the easiest way to open DOSBox configuration file is by launching it from the `Start Menu` indicated by the red ellipsis below. So, go ahead and open it:

![dosbox-configuration-file](https://github.com/user-attachments/assets/fde27dc2-8c2e-4202-b202-edf3ae9603e4)

#### Configure the layout of the keyboard

Usually, on Windows, the keyboard layout detection on DOSBox works nicely. In case the default setting does not work for you, to solve this, open DOSBox configuration file on your text editor, scroll down to the `[dos]` section, and set the field `keyboardlayout` according to the page [https://www.dosbox.com/wiki/KEYB](https://www.dosbox.com/wiki/KEYB).

For instance, since I'm brazillian, I could set it manually in the config file just by setting `keyboardlayout=br` below the `[dos]` section.

#### Configure the drive to be mounted on DOSBox

This section is optional, but since you may have to execute the command many times, I like to have it added on my configuration file so that I don't need to keep repeating it over and over.

Again, open DOSBox configuration file on your text editor. Then scroll down to the `[autoexec]` section. There, add a line to automatically mount a `C:` drive binding it to the base directory `%SystemDrive%\dosbox`, then another line to change directory to the newly mounted `C:` drive:

```batch
mount C C:\dosbox
C:
```

> [!NOTE]
> 
> Replace `C:\dosbox` above by the path of directory pointed by `%SystemDrive%\dosbox`.

By default, everytime, the DOSBox shell launchs on its own `Z:` drive without any directory mounted. Thus, the mount command above binds `C:\` (root C drive on DOS) to your `%SystemDrive%\dosbox` directory, so you don't need to repeat it again.

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
mount C C:\dosbox
C:
path C:\TC\BIN;C:\
```

Again, you shall replace `C:\dosbox` above by the path of directory pointed by `%SystemDrive%\dosbox`

## Inside DOSBox

If you got this far, you are indeed a brave adventurer passionate about exploring Lua on such ancient operating systems like the one DOSBox offers us (and potentially trying `lua-pcg` there).

So, go ahead and launch DOSBox.

> [!TIP]
> 
> In the same spirit as Windows, the filesystem provided by DOSBox is case insensitive on paths. So, the paths `C:\TC-SRC` and `c:\tc-src` are exactly the same within the DOSBox environment.

You should see this startup screen

![001-dosbox-startup](https://github.com/user-attachments/assets/a8386b5f-7379-429e-ab10-d8dca1aec85c)

### Install Turbo C++

1. Once DOSBox shell loads and performs out instructions on `[autoexec]` above, change directory to `tc-src` (like a Unix shell, DOSBox shell also accepts tab completions). The directory `tc-src` contains the zip files and bootstrap applications to install Turbo C++ (use the autocomplete while typing).

    ```batch
    cd C:\TC-SRC
    ```

2. Execute the `install.exe` application:

    ```batch
    INSTALL.EXE
    ```

    ![002-cd-tc-src-invoke-install](https://github.com/user-attachments/assets/bd9b2225-8035-4920-ba88-391fbf25c44b)


4. Once it loaded, you should see this screen

    ![003-install-initial-screen](https://github.com/user-attachments/assets/7cd1b3b5-a5b5-469a-b34f-de586dea34c2)

    Press enter.

6. When asked for the SOURCE drive to use, type `C`:

    ![004-enter-drive-c](https://github.com/user-attachments/assets/d2b6fb05-1c49-4ff1-846a-cd780decd1da)

    Press enter.

8. The installation manager of Turbo C++ will ask where the source files can be found (directory containing zip files). In case it is not there, type `\TC-SRC`:

    ![005-tc-src-directory](https://github.com/user-attachments/assets/93dd03d1-a3bc-432d-88c5-4f79475569e3)

    Press enter.

10. The next screen asks for you to define several paths:

    ![006-initial-turbo-cpp-install-directory](https://github.com/user-attachments/assets/4eef531e-dd86-4dd3-b679-1a80c4c820ab)

    We are going to accept the defaults. Then hit the `arrow down` on your keyboard to select `Start Installation` option.

> [!IMPORTANT]
> 
> **DO NOT** change defaults. Our `lua-bld.bat` depends on these paths.

12. Once the `Start Installation` option is selected, hit enter:

    ![007-scroll-down-start-installation](https://github.com/user-attachments/assets/58f804ad-4378-401f-8745-628c1d636ea2)

14. On a successfull installation, we see this screen:

    ![008-installation-success](https://github.com/user-attachments/assets/f17d55c4-7292-419e-8ef3-6ff75750f537)

    Press enter.

16. On the next screen, we can see this:

    ![009-after-pressing-enter](https://github.com/user-attachments/assets/7e150e44-daa2-4bf2-b374-832b760d37b1)

    Press enter.

18. At this moment, everything should have been installed. Run the command

    ```batch
    tcc
    ```

    to test Turbo C++ compiler:

    ![010-test-turbo-c-compiler](https://github.com/user-attachments/assets/f856a6d1-153c-4dc4-9eb3-201139cc8aab)

20. Check Turbo C++ compiler output:

    ![011-turbo-c-compiler-output](https://github.com/user-attachments/assets/7c9b0e3f-3420-4648-ac75-1504e7f5e9b1)

22. It is a good idea to also test Turbo Link (the linker):

    ```batch
    tlink
    ```

    You should see this output:

    ![012-turbo-link-output](https://github.com/user-attachments/assets/e2d466ff-480a-42d0-8381-59b3f00eb522)

### Build Lua 5.1.5 and lua-pcg

Now, we are going to build the Lua interpreter `lua.exe`.

1. Just call the bat file:

    ```batch
    C:\LUA\LUA-BLD.BAT
    ```

    ![013-call-lua-bld-bat](https://github.com/user-attachments/assets/d810f722-66a0-4fad-ae75-c02f7faaa4f2)

3. If everything was successfull, you should see this screen:

    ![014-end-of-build](https://github.com/user-attachments/assets/de6064b2-41d7-45f0-9fa5-9233df85d606)

### Run simple tests on the Lua interpreter just built

Now, the Lua interpreter has been built merged with `lua-pcg`. Run some basic test script to see the interpreter working.

1. Check Lua version:

    ```batch
    lua -v
    ```

    ![015-lua-version](https://github.com/user-attachments/assets/348e4e64-21c1-4ebb-974d-48dd1dd00327)

3. Run some code:

    ```batch
    lua -e "for i = 1, 10 do print(i) end"
    ```

    ![016-lua-for](https://github.com/user-attachments/assets/ff129939-a66b-49ce-bd31-08061a5c3158)

5. Check that `lua-pcg` was also bundled with Lua:

    ```batch
    lua -e "print(pcg ~= nil and pcg.pcg32 ~= nil and pcg.pcg64 ~= nil)"
    ```

    This should print `true` like the image below

    ![017-lua-pcg-was-bundled](https://github.com/user-attachments/assets/ad00ff86-4cfb-4a3d-a694-12f2d61268e5)

### Run lua-pcg test suite

> [!WARNING]
> 
> On my machine (*I ran the test suite completely twice on Linux*), the whole test suite takes more than eight (8) hours to run completely. Around one (1) hour in the `test32.lua` file to test `pcg32` class, and the rest on `test64.lua` to test `pcg64` class.

Since you have been properly warned that testing takes too long, go ahead if you want.

1. Change directory to `lua-pcg` directory:

    ```batch
    cd C:\LUA-PCG
    ```

    ![018-cd-lua-pcg](https://github.com/user-attachments/assets/c29897b3-cccc-45f2-8ee9-39609527a4d0)

3. Run `test32.lua` to test `pcg32` class:

    ```batch
    lua TESTS\TEST32.LUA
    ```

    ![019-run-test32](https://github.com/user-attachments/assets/96508fb6-a1e6-4267-8a69-b6b88f5b873e)

    Now, it is going to take some decent amount of time (around 1h on my computer). If you are patient enough to wait them, at the end of the tests, you can see the following screen:

    ![020-test32-completed](https://github.com/user-attachments/assets/bae1cc82-9387-4ba8-9de3-940438c4f631)

5. When it finishes, go ahead and `test64.lua` to test `pcg64` class:

    ```batch
    lua TESTS\TEST64.LUA
    ```

    ![021-run-test64](https://github.com/user-attachments/assets/772cc205-87ac-4735-a741-0b8fddd848a0)

    On my computer, it takes 7h or more to finish.

7. In case any of the tests failed, please, open an issue.

---

[Back to TOC](#table-of-contents) | [Back to docs](./README.md) | [Back to home](../)
