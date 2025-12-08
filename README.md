# TitaniumSrc<img src="https://raw.githubusercontent.com/TitaniumSrc/TitaniumSrc/master/internal/engine/resources/icons/logo.png" align="right" height="120"/>
**A community-developed fork of [PlatinumSrc](https://github.com/PlatinumSrc/PlatinumSrc)**<br>
Progress can be found [here](TODO.md)

---
- [Platform Support](#platform-support)
- [How to run](#how-to-run)
- [Building from source](#building-from-source)

---
### Platform Support
<details open><summary><b>Supported</b></summary>

- Linux
- OpenBSD
- Windows 2000+
- Windows 98
- MacOS
- HaikuOS
- Emscripten
</details>
<details open><summary><b>Untested</b></summary>

- FreeBSD
- NetBSD
</details>

---
### How to run
<details open><summary><b>Running the engine</b></summary>

1. Download a game \(the engine will not run without a game\)
    <!-- [insert game](...) -->
2. Drop the game into a directory called `games` and use the `-game` option, or ensure the `defaultgame` variable in `internal/engine/config.cfg` is set to the game's directory name
3. Put any mods into a directory called `mods` and use the `-mods` option, or ensure they are listed in the `mods` variable in one of the configs
    - You can use `config.cfg` in `internal/engine/` or in the game's user data directory
    - Mods are listed as comma-separated values without spaces between values
4. Run the executable
</details>

---
### Building from source
<details><summary><b>Building on Unix-like platforms for that same platform</b></summary>

- Install GNU Make
- Install GCC with GNU Binutils, or Clang with LLVM
    - Pass `TOOLCHAIN=llvm- CC=clang` to the Makefile to use Clang
    - On 32-bit HaikuOS, pass `CC=gcc-x86` to the Makefile to use the correct GCC executable
- Install SDL 2.x or 1.2.x
- If building the dedicated server, pass `MODULE=server` to the Makefile, or if building the editor, pass `MODULE=editor`
</details>
<details><summary><b>Building for Windows</b></summary>

- If cross-compiling on a Unix-like platform
    - Install GNU Make
    - Install MinGW
    - Install MinGW SDL 2.x or 1.2.x
    - Pass `CROSS=win32` to the Makefile
- If MSYS2 is supported
    - Install MSYS2 and use the MINGW64 backend
    - Install GNU Make
    - Install GCC with GNU Binutils, or Clang with LLVM
        - Pass `TOOLCHAIN=llvm- CC=clang` to the Makefile to use Clang
    - Install MinGW SDL 2.x or 1.2.x
- If MSYS2 is not supported
    - Install Git bash
    - Install [Make for Windows](https://sourceforge.net/projects/gnuwin32/files/make/3.81/make-3.81.exe/download) and add it to the `PATH`
    - Download MinGW and add it to the `PATH`
    - Donwload and extract MinGW SDL 2.x or 1.2.x into MinGW
- If building the dedicated server, pass `MODULE=server` to the Makefile, or if building the editor, pass `MODULE=editor`
</details>
<details><summary><b>Building for older Windows</b></summary>

- Download [MinGW 7.1.0 win32 sjlj](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/7.1.0/threads-win32/sjlj/i686-7.1.0-release-win32-sjlj-rt_v5-rev2.7z/download) and add it to the `PATH`
    - It might work with other versions but they need to not require `___mb_cur_max_func` from msvcrt.dll or `AddVectoredExceptionHandler` from kernel32.dll
- If cross-compiling on a Unix-like platform
    - Install Wine
    - Pass `CROSS=win32 TOOLCHAIN='wine '` to the Makefile
- If bulding for Windows 2000
    - Download [psrc-sdl2 MinGW 7.1.0 build](https://github.com/PQCraft/psrc-sdl2/releases/latest/download/SDL2-devel-2.29.0-mingw-7.1.0.zip), and extract it to `external/Windows_i686`
- If building for Windows 98
    - Download [SDL 1.2.x modified to be compatible with Windows 98](https://github.com/PQCraft/PQCraft/raw/master/SDL_1_2_Win98.zip), and extract it to `external/Windows_i686`
    - Pass `USESDL1=y MTLVL=1` to the Makefile
- If building the dedicated server, pass `MODULE=server` to the Makefile, or if building the editor, pass `MODULE=editor`
</details>
<details><summary><b>Building for web browsers using Emscripten</b></summary>

- Install GNU Make
- Install Emscripten
- Pass `CROSS=emscr` to the Makefile
</details>

———
<details><summary><b>Full Makefile usage</b></summary>

- Rules
    - `build` - Build an executable or ROM
    - `run` - Build an executable or ROM and run it
    - `clean` - Clean up intermediate files
    - `distclean` - Clean up intermediate and output files
    - `externclean` - Clean up external tools
- Variables
    - Build options
        - `MODULE` - Which module to build \(default is `engine`\)
            - `engine` - Game engine
            - `server` - Standalone server
            - `editor` - Map editor
        - `CROSS` - Cross compile
            - `win32` - Windows 2000+ or Windows 98 with KernelEx
            - `android` - Android
            - `emscr` - Emscripten
        - `ONLYBIN` - Set to `y` to skip making a disc image.
        - `O` - Set the optimization level \(default is `2` if `DEBUG` is unset or `g` if `DEBUG` is set\)
        - `M32` - Set to `y` to produce a 32-bit binary
        - `NATIVE` - Set to `y` to tune the build for the native system
        - `DEBUG` - Enable debug symbols and messages
            - `0` - Symbols only
            - `1` - Basic messages
            - `2` - Advanced messages
            - `3` - Detailed messages
        - `ASAN` - Set to `y` to enable the address sanitizer \(requires `DEBUG` to be set\)
        - `NOSTRIP` - Set to `y` to not strip symbols
        - `NOLTO` - Set to `y` to disable link-time optimization \(ignored if `DEBUG` is set\)
        - `NOGCSECTIONS` - Set to `y` to disable `-Wl,--gc-sections` \(ignored if `DEBUG` is set\)
        - `NOFASTMATH` - Set to `y` to disable `-ffast-math`
        - `NOSIMD` - Set to `y` to not use SIMD
        - `MT` - Set the amount of multithreading to use
            - `0` - Disabled
            - `1` - Limited
            - `2` - Full
    - Features and backends
        - `USESTDIODS` - Set to `y` to use `fopen()`, `fread()`, and `fclose()` in place of `open()`, `read()`, and `close()` in the datastream code
        - `USESDLDS` - Set to `y` to use SDL_RWops functions in place of `open()`, `read()`, and `close()` in the datastream code
        - `USEDISCORDGAMESDK` - Set to `y` to include the Discord Game SDK
        - `USEGL` - Set to `y` to include OpenGL support
        - `USEGL11` - Set to `y` to include OpenGL 1.1 support
        - `USEGL33` - Set to `y` to include OpenGL 3.3 support
        - `USEGLES30` - Set to `y` to include OpenGL ES 3.0 support
        - `USEGLAD` - Set to `y` to use glad instead of the system's GL library directly
        - `USEWEAKGL` - Set to `y` to mark `gl[A-Z]*` symbols as weak
        - `USESDL1` - Set to `y` to use SDL 1.2.x instead of SDL 2.x
        - `USESTATICSDL` - Set to `y` to statically link to SDL
        - `USEMINIMP3` - Set to `y` to include MiniMP3 for MP3 support
        - `USESTBVORBIS` - Set to `y` to include stb_vorbis for OGG Vorbis support
        - `USESTDTHREAD` - Set to `y` to use C11 threads
        - Windows
            - `USEWINPTHREAD` - Set to `y` to use winpthread instead of win32 threads
    - Toolchain options
        - `CC` - C compiler
        - `LD` - Linker \(defaults to `CC`'s value\)
        - `AR` - Archiver
        - `STRIP` - Symbol remover
        - `OBJCOPY` - Executable editor
        - `TOOLCHAIN` - Text to prepend to tool names
        - `CFLAGS` - Extra C compiler flags
        - `CPPFLAGS` - Extra C preprocessor flags
        - `LDFLAGS` - Extra linker flags
        - `LDLIBS` - Extra linker libraries
        - `RUNFLAGS` - Flags to pass to the executable
        - `EMULATOR` - Command used to run the executable or ROM
        - `EMUFLAGS` - Flags to pass to the emulator
        - `EMUPATHFLAG` - Flag used to specify the executable or ROM path
        - Windows
            - `WINDRES` - Windows resource compiler
        - Emscripten
            - `EMSCR_SHELL` - Path to the shell file
</details>

Examples:
```
make -j$(nproc)
```
```
make -j$(nproc) run
```
```
make DEBUG=1 ASAN=y -j$(nproc) run
```
```
make CROSS=nxdk DEBUG=0 -j$(nproc) run
```
