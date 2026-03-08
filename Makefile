# -------- CONFIG --------
VCPKG_ROOT   = C:/vcpkg
INC          = $(VCPKG_ROOT)/installed/x64-windows/include
VLIB_DEBUG   = $(VCPKG_ROOT)/installed/x64-windows/debug/lib

SRC      = src/main.cpp
OUT_DIR  = build
OUT_EXE  = $(OUT_DIR)/app.exe

CC      = cl
CFLAGS  = /nologo /EHsc /Zi /MDd /std:c++20 \
          /I"$(INC)" \
          /I"$(CURDIR)/include" \
          /I"C:/raylib/raygui/src" \
          /I"$(CURDIR)/src" \
          /Fo"$(OUT_DIR)/" \
          /Fd"$(OUT_DIR)/app.pdb"

LDFLAGS = /link /DEBUG /OUT:"$(OUT_EXE)" \
          /LIBPATH:"$(VLIB_DEBUG)" \
          raylib.lib winmm.lib gdi32.lib user32.lib shell32.lib




# -------- PHONY --------
.PHONY: all clean debugrun FORCE

# -------- BUILD --------
all: $(OUT_EXE)

$(OUT_DIR):
	if not exist "$(OUT_DIR)" mkdir "$(OUT_DIR)"

# FORCE ensures full rebuild every time
$(OUT_EXE): FORCE | $(OUT_DIR)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS)

clean:
	rmdir /s /q "$(OUT_DIR)" 2>NUL || echo Make clean done.

debugrun: clean all

FORCE:
