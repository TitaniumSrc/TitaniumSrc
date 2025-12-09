SRCDIR ?= src
OBJDIR ?= obj
OUTDIR ?= .
TISRCDIR ?= ../../src

CSOURCES ?= $(wildcard $(SRCDIR)/*.c)
COBJECTS ?= $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.c.o,$(SOURCES))
CXXSOURCES ?= $(wildcard $(SRCDIR)/*.cpp)
CXXOBJECTS ?= $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.cpp.o,$(SOURCES))

BIN ?= out

TARGET := $(OUTDIR)/$(BIN)
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
endif

CC ?= gcc
CXX ?= g++
LD := $(CXX)
STRIP ?= strip
_CC := $(TOOLCHAIN)$(CC)
_CXX := $(TOOLCHAIN)$(CXX)
_LD := $(TOOLCHAIN)$(LD)
_STRIP := $(TOOLCHAIN)$(STRIP)

_CFLAGS := $(CFLAGS) -Wall -Wextra -Wuninitialized -Wundef
_CXXFLAGS := $(CXXFLAGS) -fno-rtti -fno-exceptions
_CPPFLAGS := $(CPPFLAGS) -I$(TISRCDIR) -DTISRC_REUSABLE
_LDFLAGS := $(LDFLAGS)
_LDLIBS := $(LDLIBS)
ifneq ($(DEBUG),y)
    _CPPFLAGS += -DNDEBUG
    O ?= 2
else
    _CFLAGS += -g -Wdouble-promotion -fno-omit-frame-pointer
    O ?= g
endif
_CFLAGS += -O$(O)
_CXXFLAGS += $(_CFLAGS)
ifeq ($(DEBUG),y)
    _CFLAGS += -std=c11 -pedantic
endif
ifeq ($(ASAN),y)
    _CFLAGS += -fsanitize=address
    _LDFLAGS += -fsanitize=address
endif

.SECONDEXPANSION:

define mkdir
if [ ! -d '$(1)' ]; then echo 'Creating $(1)/...'; mkdir -p '$(1)'; fi; true
endef
define rm
if [ -f '$(1)' ]; then echo 'Removing $(1)...'; rm -f '$(1)'; fi; true
endef
define rmdir
if [ -d '$(1)' ]; then echo 'Removing $(1)/...'; rm -rf '$(1)'; fi; true
endef

deps.filter := %.c %.cpp %.h %.hpp
deps.option := -MM
define cdeps
$$(filter $$(deps.filter),,$$(shell $(_CC) $(_CFLAGS) $(_CPPFLAGS) -E $(deps.option) $(1)))
endef
define cxxdeps
$$(filter $$(deps.filter),,$$(shell $(_CC) $(_CPPFLAGS) $(_CPPFLAGS) -E $(deps.option) $(1)))
endef

default: build

$(OUTDIR):
	@$(call mkdir,$@)

$(OBJDIR):
	@$(call mkdir,$@)

$(OBJDIR)/%.c.o: $(SRCDIR)/%.c $(call cdeps,$(SRCDIR)/%.c) | $(OBJDIR) $(OUTDIR)
	@echo Compiling $<...
	@$(_CC) $(_CFLAGS) $(_CPPFLAGS) $< -c -o $@
	@echo Compiled $<

$(OBJDIR)/%.cpp.o: $(SRCDIR)/%.cpp $(call cxxdeps,$(SRCDIR)/%.cpp) | $(OBJDIR) $(OUTDIR)
	@echo Compiling $<...
	@$(_CXX) $(_CXXFLAGS) $(_CPPFLAGS) $< -c -o $@
	@echo Compiled $<

$(TARGET): $(COBJECTS) $(CXXOBJECTS) | $(OUTDIR)
	@echo Linking $@...
	@$(_LD) $(_LDFLAGS) $^ $(_LDLIBS) -o $@
ifneq ($(NOSTRIP),y)
	@$(_STRIP) -s -R '.comment' -R '.note.*' -R '.gnu.build-id' $@ || exit 0
endif
	@echo Linked $@

build: $(TARGET)
	@:

clean:
	@$(call rmdir,$(OBJDIR))

distclean: clean
	@$(call rm,$(TARGET))

.PHONY: default build clean distclean
