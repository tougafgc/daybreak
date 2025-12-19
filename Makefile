# Makefile
# By: tougafgc
# Date: 13 November 2025
#
# Creates the executables and extensions
# needed for Daybreak.
#

### Defines
CXX       = g++ #i686-w64-mingw32-g++
EXE_FLAGS = -static-libgcc -static-libstdc++
DLL_FLAGS = -shared -static-libgcc -static-libstdc++ -Llib/squirrel/lib -lsquirrel -lsqstdlib -ld3d9 -ld3dx9 -lgdi32 -ldwmapi
CPPFLAGS  = -Wall --std=c++17 -m32 -DWINDOWS
EXE_OUT   = out/daybreak.exe
DLL_OUT   = out/daybreak/crescent.dll

EXE_DIR = exe
DLL_DIR = dll

EXE_FILES = $(wildcard $(EXE_DIR)/*.cpp) main.cpp
EXE_OBJS  = $(patsubst $(EXE_DIR)/%.cpp, $(EXE_DIR)/%.o, $(filter-out main.cpp, $(EXE_FILES))) main.o

DLL_FILES = $(wildcard $(DLL_DIR)/*.cpp) dll.cpp
DLL_OBJS  = $(patsubst $(DLL_DIR)/%.cpp, $(DLL_DIR)/%.o, $(filter-out dll.cpp, $(DLL_FILES))) dll.o

IMGUI_FILES = $(wildcard lib/imgui/*.cpp)
IMGUI_OBJS  = $(patsubst lib/imgui/%.cpp, lib/imgui/%.o, $(IMGUI_FILES))

.PHONY: first all init out test update clean

### Major processes
first: init $(EXE_OUT) $(DLL_OUT) out

all: $(EXE_OUT) $(DLL_OUT) out

init:
	git submodule update --init
	cd lib/squirrel && $(MAKE) sq32
	cp lib/imgui/backends/imgui_impl_dx9.cpp lib/imgui/imgui_impl_dx9.cpp
	cp lib/imgui/backends/imgui_impl_dx9.h lib/imgui/imgui_impl_dx9.h
	cp lib/imgui/backends/imgui_impl_win32.cpp lib/imgui/imgui_impl_win32.cpp
	cp lib/imgui/backends/imgui_impl_win32.h lib/imgui/imgui_impl_win32.h

out:
	cp /mingw32/bin/libwinpthread-1.dll out/libwinpthread-1.dll
	cp crescent/newlisp.dll   out/daybreak/newlisp.dll
	cp crescent/newlisp.exe   out/daybreak/newlisp.exe
	cp crescent/server.lsp    out/daybreak/server.lsp
	cp crescent/client.lsp    out/daybreak/client.lsp
	cp crescent/autorun.crsc  out/daybreak/autorun.crsc
	cp -r out/daybreak mbaa/daybreak
	cp -r out/daybreak.exe mbaa/daybreak.exe
	cp -r out/libwinpthread-1.dll mbaa/libwinpthread-1.dll
	./crescent/newlisp.exe -x crescent/client.lsp out/daybreak/client.exe

test:
	cd mbaa && ./daybreak.exe

update:
	git pull
	git submodule update --remote

clean:
	rm -rf mbaa/daybreak mbaa/daybreak.exe
	rm -rf out $(DLL_DIR)/*.o $(EXE_DIR)/*.o lib/imgui/*.o *.o

### Compilation and linking
$(EXE_OUT): $(EXE_OBJS)
	mkdir -p out
	mkdir -p out/daybreak
	$(CXX) $^ -o $@ $(EXE_FLAGS)

$(DLL_OUT): $(DLL_OBJS)  $(IMGUI_OBJS)
	$(CXX) $^ -o $@ $(DLL_FLAGS)

%.o: %.cpp $(EXE_FILES) $(DLL_FILES) $(IMGUI_FILES)
	$(CXX) -c $(CPPFLAGS) $< -o $@
