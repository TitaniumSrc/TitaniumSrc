default: build
	@:

null := /dev/null

MODULE := engine
SRCDIR := src
OBJDIR := obj
EXTDIR := external
OUTDIR := .

ifeq ($(OS),Windows_NT)
    CROSS := win32
endif

ifeq ($(CROSS),)
    CC ?= gcc
    LD := $(CC)
    STRIP ?= strip
    OBJCOPY ?= objcopy
    _CC := $(TOOLCHAIN)$(CC)
    _LD := $(TOOLCHAIN)$(LD)
    _STRIP := $(TOOLCHAIN)$(STRIP)
    _OBJCOPY := $(TOOLCHAIN)$(OBJCOPY)
    ifneq ($(M32),y)
        PLATFORM := $(subst $() $(),_,$(subst /,_,$(shell uname -o 2> $(null) || uname -s; uname -m)))
    else
        PLATFORM := $(subst $() $(),_,$(subst /,_,$(shell i386 uname -o 2> $(null) || i386 uname -s; i386 uname -m)))
    endif
    USESR := y
    USEGL := y
    KERNEL := $(shell uname -s)
    ifeq ($(KERNEL),Darwin)
        USEGLAD := y
        NOGCSECTIONS := y
    endif
    ifneq ($(USEGLAD),y)
        USEWEAKGL := y
    endif
else ifeq ($(CROSS),win32)
    ifneq ($(M32),y)
        ifneq ($(OS),Windows_NT)
            TOOLCHAIN := x86_64-w64-mingw32-
        endif
        PLATFORM := Windows_x86_64
    else
        ifneq ($(OS),Windows_NT)
            TOOLCHAIN := i686-w64-mingw32-
        endif
        PLATFORM := Windows_i686
    endif
    CC := gcc
    LD := $(CC)
    STRIP := strip
    OBJCOPY := objcopy
    WINDRES := windres
    _CC := $(TOOLCHAIN)$(CC)
    _LD := $(TOOLCHAIN)$(LD)
    _STRIP := $(TOOLCHAIN)$(STRIP)
    _OBJCOPY := $(TOOLCHAIN)$(OBJCOPY)
    _WINDRES := $(TOOLCHAIN)$(WINDRES)
    USESR := y
    USEGL := y
    USEGLAD := y
    USESTATICSDL := y
else ifeq ($(CROSS),android)
    ifeq ($(MODULE),engine)
    else ifneq ($(MODULE),editor)
        $(error Invalid module: $(MODULE))
    endif
    PLATFORM := Android
    _CC := $(CC)
    _LD := $(_CC)
    NOLTO := y
    NOSTRIP := y
    NOGCSECTIONS := y
    USEGLES30 := y
    USEGLAD := y
    USESTDTHREAD := y
    USESDLDS := y
else ifeq ($(CROSS),emscr)
    ifeq ($(MODULE),engine)
    else ifneq ($(MODULE),editor)
        $(error Invalid module: $(MODULE))
    endif
    PLATFORM := Emscripten
    _CC := emcc
    _LD := $(_CC)
    EMULATOR := emrun
    EMUPATHFLAG := --
    NOSTRIP := y
    MT := 0
    USEGL11 := y
else
    $(error Invalid cross-compilation target: $(CROSS))
endif

ifeq ($(MODULE),engine)
    USEMINIMP3 := y
    USESTBVORBIS := y
    USEPLMPEG := y
else ifeq ($(MODULE),server)
else ifeq ($(MODULE),editor)
    USEMINIMP3 := y
    USESTBVORBIS := y
    USEPLMPEG := y
else
    $(error Invalid module: $(MODULE))
endif

PLATFORMDIRNAME := $(MODULE)
ifeq ($(USEDISCORDGAMESDK),y)
    PLATFORMDIRNAME := $(PLATFORMDIRNAME)_discordgsdk
endif
ifndef DEBUG
    PLATFORMDIR := release/$(PLATFORM)
else
    ifeq ($(ASAN),y)
        PLATFORMDIR := debug_asan/$(PLATFORM)
    else
        PLATFORMDIR := debug/$(PLATFORM)
    endif
endif
ifndef DEBUG
    ifneq ($(NOLTO),y)
        PLATFORMDIR := $(PLATFORMDIR)_LTO
    endif
endif
PLATFORMDIR := $(PLATFORMDIR)/$(PLATFORMDIRNAME)
_OBJDIR := $(OBJDIR)/$(PLATFORMDIR)

ifeq ($(CROSS),win32)
    SOSUF := .dll
else
    SOSUF := .so
endif

ifeq ($(USEGL),y)
    USEGL11 := y
    USEGL33 := y
    USEGLES30 := y
else ifeq ($(USEGL11),y)
    USEGL := y
else ifeq ($(USEGL33),y)
    USEGL := y
else ifeq ($(USEGLES30),y)
    USEGL := y
endif
ifeq ($(MTLVL),)
    MTLVL := 2
endif

_CFLAGS := $(CFLAGS) -I$(EXTDIR)/$(PLATFORM)/include -I$(EXTDIR)/include -fno-exceptions -Wall -Wextra -Wuninitialized -Wundef -fvisibility=hidden
_CPPFLAGS := $(CPPFLAGS) -DPSRC_MTLVL=$(MTLVL) -D_DEFAULT_SOURCE -D_GNU_SOURCE
_LDFLAGS := $(LDFLAGS) -L$(EXTDIR)/$(PLATFORM)/lib -L$(EXTDIR)/lib
_LDLIBS := $(LDLIBS) -lm
_WRFLAGS := $(WRFLAGS)
ifneq ($(NOFASTMATH),y)
    _CFLAGS += -ffast-math
    _LDFLAGS += -ffast-math
endif
ifeq ($(CROSS),)
    _CFLAGS += -I/usr/local/include
    _LDFLAGS += -L/usr/local/lib
    ifeq ($(KERNEL),Darwin)
        _CFLAGS += -I/opt/homebrew/include
        _LDFLAGS += -L/opt/homebrew/lib
    endif
else ifeq ($(CROSS),win32)
    _LDFLAGS += -static -static-libgcc
else ifeq ($(CROSS),android)
    _CFLAGS += -fPIC -fcf-protection=none
    #_CFLAGS += -fno-stack-clash-protection
    _LDFLAGS += -shared
    _LDLIBS += -llog -landroid
else ifeq ($(CROSS),emscr)
    _LDFLAGS += -sALLOW_MEMORY_GROWTH -sEXIT_RUNTIME -sEXPORTED_RUNTIME_METHODS=ccall -sWASM_BIGINT
    ifndef EMSCR_SHELL
        _LDFLAGS += --shell-file $(SRCDIR)/psrc/emscr_shell.html
    else
        _LDFLAGS += --shell-file $(EMSCR_SHELL)
    endif
    _LDLIBS += -lidbfs.js
    ifeq ($(USEGL),y)
        _LDFLAGS += -sLEGACY_GL_EMULATION -sGL_UNSAFE_OPTS=0
    endif
    _LDFLAGS += --embed-file internal/engine/ --embed-file internal/server/ --embed-file games/ --embed-file mods/
else ifeq ($(CROSS),3ds)
    _CFLAGS += -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -mword-relocations -ffunction-sections -I$(DEVKITPRO)/libctru/include -I$(DEVKITPRO)/portlibs/3ds/include
    _CPPFLAGS += -D__3DS__
    _LDFLAGS += -specs=3dsx.specs -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -L$(DEVKITPRO)/libctru/lib -L$(DEVKITPRO)/portlibs/3ds/lib
    _LDLIBS += -lcitro2d -lcitro3d
else ifeq ($(CROSS),wii)
    _CFLAGS += -mrvl -mcpu=750 -meabi -mhard-float -I$(DEVKITPRO)/libogc/include -I$(DEVKITPRO)/portlibs/wii/include
    _CPPFLAGS += -DGEKKO -D__wii__
    _LDFLAGS += -mrvl -mcpu=750 -meabi -mhard-float -L$(DEVKITPRO)/libogc/lib/wii -L$(DEVKITPRO)/portlibs/wii/lib
    _LDLIBS += -lfat
else ifeq ($(CROSS),gc)
    _CFLAGS += -mogc -mcpu=750 -meabi -mhard-float -I$(DEVKITPRO)/libogc/include -I$(DEVKITPRO)/portlibs/gamecube/include
    _CPPFLAGS += -DGEKKO -D__gamecube__
    _LDFLAGS += -mogc -mcpu=750 -meabi -mhard-float -L$(DEVKITPRO)/libogc/lib/cube -L$(DEVKITPRO)/portlibs/gamecube/lib
endif
ifeq ($(USEGL),y)
    ifeq ($(USEGLAD),y)
        _CPPFLAGS += -DPSRC_ENGINE_RENDERER_GL_USEGLAD
    else
        _LDLIBS += -lGL
    endif
endif
ifneq ($(MT),0)
    ifneq ($(USESTDTHREAD),y)
        ifneq ($(CROSS),win32)
            ifneq ($(CROSS),dc)
                _CFLAGS += -pthread
            endif
            _LDLIBS += -lpthread
        else
            ifeq ($(USEWINPTHREAD),y)
                _CFLAGS += -pthread
                _CPPFLAGS += -DPSRC_THREADING_USEWINPTHREAD
                _LDLIBS += -l:libwinpthread.a
            endif
        endif
    endif
endif
ifneq ($(CROSS),emscr)
    ifeq ($(M32),y)
        _CFLAGS += -m32
        _CPPFLAGS += -DM32
        _LDFLAGS += -m32
        ifeq ($(CROSS),win32)
            _WRFLAGS += -DM32
        endif
    endif
    ifdef DEBUG
        #_LDFLAGS += -Wl,-R$(EXTDIR)/$(PLATFORM)/lib -Wl,-R$(EXTDIR)/lib
    endif
    ifeq ($(NATIVE),y)
        _CFLAGS += -march=native -mtune=native
    endif
endif
ifneq ($(DEFAULTGAME),)
    _CPPFLAGS += -DPSRC_DEFAULTGAME='$(subst ','\'',$(DEFAULTGAME))'
endif
ifeq ($(NOSIMD),y)
    _CPPFLAGS += -DPSRC_NOSIMD
endif
ifeq ($(USESTDIODS),y)
    _CPPFLAGS += -DPSRC_DATASTREAM_USESTDIO
endif
ifeq ($(USESDLDS),y)
    _CPPFLAGS += -DPSRC_DATASTREAM_USESDL
endif
ifeq ($(USEMINIMP3),y)
    _CPPFLAGS += -DPSRC_USEMINIMP3
endif
ifeq ($(USESTBVORBIS),y)
    _CPPFLAGS += -DPSRC_USESTBVORBIS
endif
ifeq ($(USEPLMPEG),y)
    _CPPFLAGS += -DPSRC_USEPLMPEG
endif
ifeq ($(USESR),y)
    _CPPFLAGS += -DPSRC_ENGINE_RENDERER_USESR
endif
ifeq ($(USEGL),y)
    _CPPFLAGS += -DPSRC_ENGINE_RENDERER_USEGL
endif
ifeq ($(USEGL11),y)
    _CPPFLAGS += -DPSRC_ENGINE_RENDERER_GL_USEGL11
endif
ifeq ($(USEGL33),y)
    _CPPFLAGS += -DPSRC_ENGINE_RENDERER_GL_USEGL33
endif
ifeq ($(USEGLES30),y)
    _CPPFLAGS += -DPSRC_ENGINE_RENDERER_GL_USEGLES30
endif
ifndef DEBUG
    _CPPFLAGS += -DNDEBUG=1
    ifneq ($(NOGCSECTIONS),y)
        _LDFLAGS += -Wl,--gc-sections
    endif
    ifndef O
        O := 2
        ifeq ($(CROSS),)
        else ifeq ($(CROSS),win32)
        else
            O := s
        endif
    endif
    _CFLAGS += -O$(O)
    ifeq ($(CROSS),emscr)
        _LDFLAGS += -O$(O)
    endif
    ifneq ($(NOLTO),y)
        _CFLAGS += -flto
        _LDFLAGS += -flto
    endif
else
    _CPPFLAGS += -DPSRC_DBGLVL=$(DEBUG)
    ifndef O
        O := g
    endif
    _CFLAGS += -O$(O) -g -Wdouble-promotion -fno-omit-frame-pointer
    #_CFLAGS += -Wconversion
    ifneq ($(CROSS),emscr)
        _CFLAGS += -std=c11 -pedantic
    endif
    ifeq ($(CROSS),win32)
        _WRFLAGS += -DPSRC_DBGLVL=$(DEBUG)
    endif
    NOSTRIP := y
    ifeq ($(ASAN),y)
        _CFLAGS += -fsanitize=address
        _LDFLAGS += -fsanitize=address
    endif
endif

CPPFLAGS.dir.lz4 := -DXXH_NAMESPACE=LZ4_ -DLZ4_STATIC_LINKING_ONLY_ENDIANNESS_INDEPENDENT_OUTPUT

CPPFLAGS.dir.psrc := 
ifeq ($(USESTDTHREAD),y)
    CPPFLAGS.dir.psrc += -DPSRC_THREADING_USESTDTHREAD
endif
LDLIBS.dir.psrc := 
ifeq ($(CROSS),win32)
    LDLIBS.dir.psrc += -lwinmm
endif

LDLIBS.dir.psrc_server := 
ifeq ($(CROSS),win32)
    LDLIBS.dir.psrc_server += -lws2_32
endif

ifeq ($(USEMINIMP3),y)
    CPPFLAGS.dir.minimp3 := -DMINIMP3_NO_STDIO
    ifeq ($(NOSIMD),y)
        CPPFLAGS.dir.minimp3 += -DMINIMP3_NO_SIMD
    endif
endif

ifeq ($(USESTBVORBIS),y)
    CPPFLAGS.dir.stbvorbis := -DSTB_VORBIS_NO_STDIO
endif

ifeq ($(CROSS),)
else ifeq ($(CROSS),win32)
else
    CPPFLAGS.dir.schrift := -DSCHRIFT_NO_FILE_MAPPING
endif

CPPFLAGS.dir.stb := -DSTBI_ONLY_PNG -DSTBI_ONLY_JPEG -DSTBI_ONLY_TGA -DSTBI_ONLY_BMP
ifeq ($(NOSIMD),y)
    CPPFLAGS.dir.stb += -DSTBI_NO_SIMD
endif
ifeq ($(MT),0)
    CPPFLAGS.dir.stb += -DSTBI_NO_THREAD_LOCALS
endif

ifeq ($(USEDISCORDGAMESDK),y)
    CPPFLAGS.lib.discord_game_sdk := -DPSRC_USEDISCORDGAMESDK
    LDLIBS.lib.discord_game_sdk := -l:discord_game_sdk$(SOSUF)
endif

CFLAGS.lib.SDL := 
CPPFLAGS.lib.SDL := -DSDL_MAIN_HANDLED
LDLIBS.lib.SDL := 
ifneq ($(USESDL1),y)
    ifeq ($(CROSS),emscr)
        CFLAGS.lib.SDL += -sUSE_SDL=2
        LDLIBS.lib.SDL += -sUSE_SDL=2
    else
        ifeq ($(USESTATICSDL),y)
            LDLIBS.lib.SDL += -l:libSDL2.a
        else
            LDLIBS.lib.SDL += -lSDL2
        endif
        ifeq ($(CROSS),win32)
            LDLIBS.lib.SDL += -lole32 -loleaut32 -limm32 -lsetupapi -lversion -lgdi32 -lwinmm
        else ifeq ($(CROSS),3ds)
            LDLIBS.lib.SDL += -lm
        else ifeq ($(CROSS),wii)
            LDLIBS.lib.SDL += -lwiiuse -lwiikeyboard -lbte -laesnd -lm
        else ifeq ($(CROSS),gc)
            LDLIBS.lib.SDL += -laesnd -lm
        endif
    endif
else
    CPPFLAGS.lib.SDL += -DPSRC_USESDL1
    ifeq ($(CROSS),emscr)
        CFLAGS.lib.SDL += -sUSE_SDL
        LDLIBS.lib.SDL += -sUSE_SDL
    else
        ifeq ($(USESTATICSDL),y)
            LDLIBS.lib.SDL += -l:libSDL.a
        else
            LDLIBS.lib.SDL += -lSDL
        endif
        ifeq ($(CROSS),win32)
            LDLIBS.lib.SDL += -liconv -luser32 -lgdi32 -lwinmm -ldxguid
        endif
    endif
endif

ifeq ($(MODULE),engine)
    _CPPFLAGS += -DPSRC_MODULE_ENGINE
    _WRFLAGS += -DPSRC_MODULE_ENGINE
    _CFLAGS += $(CFLAGS.lib.SDL)
    _CPPFLAGS += $(CPPFLAGS.dir.lz4) $(CPPFLAGS.dir.stb) $(CPPFLAGS.dir.minimp3) $(CPPFLAGS.dir.stbvorbis) $(CPPFLAGS.dir.schrift) $(CPPFLAGS.dir.psrc)
    _CPPFLAGS += $(CPPFLAGS.lib.SDL) $(CPPFLAGS.lib.discord_game_sdk)
    _LDLIBS += $(LDLIBS.dir.psrc_engine) $(LDLIBS.dir.psrc)
    _LDLIBS += $(LDLIBS.lib.discord_game_sdk) $(LDLIBS.lib.SDL)
else ifeq ($(MODULE),server)
    _CPPFLAGS += -DPSRC_MODULE_SERVER
    _WRFLAGS += -DPSRC_MODULE_SERVER
    _CPPFLAGS += $(CPPFLAGS.dir.lz4)
    _CPPFLAGS += $(CPPFLAGS.lib.discord_game_sdk)
    _LDLIBS += $(LDLIBS.dir.psrc) $(LDLIBS.lib.discord_game_sdk)
else ifeq ($(MODULE),editor)
    _CPPFLAGS += -DPSRC_MODULE_EDITOR
    _WRFLAGS += -DPSRC_MODULE_EDITOR
    _CFLAGS += $(CFLAGS.lib.SDL)
    _CPPFLAGS += $(CPPFLAGS.dir.lz4) $(CPPFLAGS.dir.stb) $(CPPFLAGS.dir.minimp3) $(CPPFLAGS.dir.stbvorbis) $(CPPFLAGS.dir.schrift) $(CPPFLAGS.dir.psrc)
    _CPPFLAGS += $(CPPFLAGS.lib.SDL) $(CPPFLAGS.lib.discord_game_sdk)
    _LDLIBS += $(LDLIBS.dir.psrc_engine) $(LDLIBS.dir.psrc)
    _LDLIBS += $(LDLIBS.lib.discord_game_sdk) $(LDLIBS.lib.SDL)
endif

ifeq ($(MODULE),server)
BIN := tisrc-server
else ifeq ($(MODULE),editor)
BIN := tisrc-editor
else
BIN := tisrc
endif
ifdef DEBUG
    ifneq ($(CROSS),android)
        BIN := $(BIN)_debug
        ifeq ($(ASAN),y)
            BIN := $(BIN)_asan
        endif
    endif
endif
ifeq ($(CROSS),win32)
    BINPATH := $(BIN).exe
else ifeq ($(CROSS),android)
    BINPATH := lib$(BIN).so
else ifeq ($(CROSS),emscr)
    BINPATH := index.html
else
    BINPATH := $(BIN)
endif
BINPATH := $(OUTDIR)/$(BINPATH)

ifeq ($(CROSS),win32)
    ifneq ($(_WINDRES),)
        WRSRC := $(SRCDIR)/psrc/winver.rc
        WROBJ := $(_OBJDIR)/psrc/winver.o
        _WROBJ = $$(test -f $(WROBJ) && echo $(WROBJ))
    endif
endif

TARGET := $(BINPATH)

define mkdir
for d in $(1); do if [ ! -d $$d ]; then echo Creating $$d/...; mkdir -p -- $$d; fi; done
endef
define rm
for f in $(1); do if [ -f $$f ]; then echo Removing $$f...; rm -f -- $$f; fi; done
endef
define rmdir
for d in $(1); do if [ -d $$d ]; then echo Removing $$d/...; rm -rf -- $$d; fi; done
endef

ifndef EMULATOR
define exec
'$(dir $(1))$(notdir $(1))' $(RUNFLAGS)
endef
else
define exec
$(EMULATOR) $(EMUFLAGS) $(EMUPATHFLAG) '$(dir $(1))$(notdir $(1))' $(RUNFLAGS)
endef
endif

.SECONDEXPANSION:

deps.filter := %.c %.h
deps.option := -MM
define deps
$$(filter $$(deps.filter),,$$(shell $(_CC) $(_CFLAGS) $(_CPPFLAGS) -E $(deps.option) $(1)))
endef

ifeq ($(TR),y)
    TR_FILE := timereport.txt
    _TR_BEFORE := T="$$(mktemp)";env time -f%e --output="$$T" --
    _TR_AFTER = ;R=$$?;printf '%s: %ss\n' $< $$(cat "$$T") >> $(TR_FILE);rm "$$T";exit "$$R"
    _TR_STFILE := $(shell mktemp)
$(TR_FILE):
	@printf '%s\n' '--------- BUILD TIME REPORT ---------' > $(TR_FILE)
	@date +%s%N > $(_TR_STFILE)
endif

SRCDIRS := $(SRCDIR)/psrc
ifeq ($(MODULE),engine)
    SRCDIRS := $(SRCDIRS) $(SRCDIR)/psrc/engine $(SRCDIR)/psrc/server
else ifeq ($(MODULE),server)
    SRCDIRS := $(SRCDIRS) $(SRCDIR)/psrc/server
else ifeq ($(MODULE),editor)
    SRCDIRS := $(SRCDIRS) $(SRCDIR)/psrc/engine $(SRCDIR)/psrc/server $(SRCDIR)/psrc/editor
endif
SRCDIRS := $(SRCDIRS) $(SRCDIR)/psrc/common $(SRCDIR)/psrc $(SRCDIR)/lz4
ifneq ($(MODULE),server)
    SRCDIRS := $(SRCDIRS) $(SRCDIR)/stb $(SRCDIR)/schrift
    ifeq ($(USEMINIMP3),y)
        SRCDIRS := $(SRCDIRS) $(SRCDIR)/minimp3
    endif
    ifeq ($(USEPLMPEG),y)
        SRCDIRS := $(SRCDIRS) $(SRCDIR)/pl_mpeg
    endif
    ifeq ($(USEGL),y)
        ifeq ($(USEGLAD),y)
            SRCDIRS := $(SRCDIRS) $(SRCDIR)/glad
        endif
    endif
endif

SRCDIRS := $(SRCDIRS)
SOURCES := $(wildcard $(addsuffix /*.c,$(SRCDIRS)))
OBJDIRS := $(patsubst $(SRCDIR)/%,$(_OBJDIR)/%,$(SRCDIRS))
OBJECTS := $(patsubst $(SRCDIR)/%.c,$(_OBJDIR)/%.o,$(SOURCES))

$(OUTDIR):
	@$(call mkdir,$@)

$(_OBJDIR):
	@$(call mkdir,$@ $(OBJDIRS))

$(_OBJDIR)/%.o: $(SRCDIR)/%.c $(call deps,$(SRCDIR)/%.c) | $(_OBJDIR) $(TR_FILE)
	@echo Compiling $(patsubst $(SRCDIR)/%,%,$<)...
	@$(_TR_BEFORE) $(_CC) $(_CFLAGS) $(_CPPFLAGS) $< -c -o $@ $(_TR_AFTER)
	@echo Compiled $(patsubst $(SRCDIR)/%,%,$<)

$(BINPATH): $(OBJECTS) | $(OUTDIR)
ifeq ($(TR),y)
	@sort -r -k2 $(TR_FILE) -o $(TR_FILE)
	@printf '%s\n' '-------------------------------------' >> $(TR_FILE)
	@printf 'Total time spent compiling: %ss\n' $$(sed 's/.\{2\}$$/.&/' <<< $$((($$(date +%s%N)-$$(cat $(_TR_STFILE)))/10000000))) >> $(TR_FILE)
	@rm $(_TR_STFILE)
endif
	@echo Linking $@...
ifeq ($(CROSS),win32)
ifneq ($(_WINDRES),)
	-@$(_WINDRES) $(_WRFLAGS) $(WRSRC) -o $(WROBJ)
endif
endif
ifeq ($(CROSS),emscr)
	@$(_LD) $(_LDFLAGS) $^ $(_LDLIBS) -o $(OUTDIR)/$(BIN).html
	@mv -f $(OUTDIR)/$(BIN).html $(BINPATH)
else
	@$(_LD) $(_LDFLAGS) $^ $(_WROBJ) $(_LDLIBS) -o $@
ifneq ($(NOSTRIP),y)
	-@$(_STRIP) -s -R '.comment' -R '.note.*' -R '.gnu.build-id' $@ || $(_STRIP) -s $@
endif
ifeq ($(USEWEAKGL),y)
	-@$(_OBJCOPY) -w -W 'gl[A-Z]*' $@
endif
endif
	@echo Linked $@

build: $(TARGET)
	@:

run: build
	@echo Running $(TARGET)...
	@$(call exec,$(TARGET))

clean:
ifeq ($(TR),y)
	@$(call rm,$(TR_FILE))
	@$(call rm,$(_TR_STFILE))
endif
	@$(call rmdir,$(_OBJDIR))

distclean: clean
ifeq ($(CROSS),emscr)
	@$(call rm,$(basename $(OUTDIR)/$(BIN)).js)
	@$(call rm,$(basename $(OUTDIR)/$(BIN)).wasm)
endif
	@$(call rm,$(TARGET))

.PHONY: default build run clean distclean $(_OBJDIR)
