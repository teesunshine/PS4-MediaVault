# PS4-MediaVault - modelled on the OpenOrbis sample Makefiles.
# Requires: OO_PS4_TOOLCHAIN set to the folder containing link.x
#   make setup   copy the icon, about/right.sprx and sce_module/*.prx from a toolchain sample
#   make         build the .pkg
#   make test    host-side unit tests (no toolchain needed)

TITLE       := PS4 MediaVault
VERSION     := 1.00
TITLE_ID    := BREW00099
CONTENT_ID  := IV0000-BREW00099_00-MEDIAVAULT000000

LIBS        := -lc -lkernel -lc++ -lSceVideoOut -lScePad -lSceUserService
LIBMODULES  := $(wildcard sce_module/*)

TC          := $(OO_PS4_TOOLCHAIN)
COMMONDIR   := $(TC)/samples/_common
INTDIR      := build
TOOLS       := $(TC)/bin/linux

CC          := clang
CXX         := clang++
LD          := ld.lld

CFLAGS      := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -O2 -Wall -c -isysroot $(TC) -isystem $(TC)/include
CXXFLAGS    := $(CFLAGS) -isystem $(TC)/include/c++/v1 -I$(COMMONDIR) -Isrc
LDFLAGS     := -m elf_x86_64 -pie --script $(TC)/link.x --eh-frame-hdr -L$(TC)/lib $(LIBS) $(TC)/lib/crt1.o

C_SRC       := $(wildcard src/*.c)
CXX_SRC     := $(wildcard src/*.cpp)
OBJS        := $(patsubst src/%.c,$(INTDIR)/%.o,$(C_SRC)) $(patsubst src/%.cpp,$(INTDIR)/%.o,$(CXX_SRC)) $(INTDIR)/graphics.o

all: $(CONTENT_ID).pkg

$(CONTENT_ID).pkg: pkg.gp4
	$(TOOLS)/PkgTool.Core pkg_build $< .

pkg.gp4: eboot.bin sce_sys/about/right.sprx sce_sys/param.sfo sce_sys/icon0.png $(LIBMODULES)
	$(TOOLS)/create-gp4 -out $@ --content-id=$(CONTENT_ID) --files "$^"

sce_sys/param.sfo: Makefile
	$(TOOLS)/PkgTool.Core sfo_new $@
	$(TOOLS)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(TOOLS)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(TOOLS)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(TOOLS)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(TOOLS)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(TOOLS)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(TOOLS)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(TOOLS)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(TOOLS)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(TOOLS)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

eboot.bin: $(OBJS)
	$(LD) $(OBJS) -o $(INTDIR)/mediavault.elf $(LDFLAGS)
	$(TOOLS)/create-fself -in=$(INTDIR)/mediavault.elf -out=$(INTDIR)/mediavault.oelf --eboot "eboot.bin" --paid 0x3800000000000011

$(INTDIR)/%.o: src/%.c | $(INTDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/%.o: src/%.cpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(INTDIR)/graphics.o: $(COMMONDIR)/graphics.cpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(INTDIR):
	mkdir -p $@

# Copies toolchain-provided assets (not stored in this repo) from the hello_world sample.
setup:
	mkdir -p sce_sys/about sce_module
	cp -n $(TC)/samples/hello_world/sce_sys/icon0.png sce_sys/ || true
	cp -n $(TC)/samples/hello_world/sce_sys/about/right.sprx sce_sys/about/ || true
	cp -n $(TC)/samples/hello_world/sce_module/* sce_module/ || true

# Host-side unit tests (no toolchain needed)
test:
	gcc -Wall -Wextra -Isrc tests/test_list.c src/mv_list.c -o build_test && ./build_test && rm -f build_test

clean:
	rm -rf $(INTDIR) build_test eboot.bin pkg.gp4 sce_sys/param.sfo *.pkg

.PHONY: all setup test clean
