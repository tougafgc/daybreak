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
DLL_FLAGS = -shared -fPIC -static-libgcc -static-libstdc++ -Lsrc/dll/squirrel3/lib -lsquirrel -lsqstdlib -ld3d9 -ld3dx9 -lgdi32 -ldwmapi
CPPFLAGS  = -Wall --std=c++17 -m32 -DWINDOWS
EXE_OUT   = out/daybreak.exe
DLL_OUT   = out/daybreak/daybreak.dll

SRC_DIR = src
EXE_DIR = src/exe
DLL_DIR = src/dll

SRC_FILES = $(wildcard $(EXE_DIR)/*.cpp) src/main.cpp
SRC_OBJS  = $(patsubst $(EXE_DIR)/%.cpp, $(EXE_DIR)/%.o, $(filter-out src/main.cpp, $(SRC_FILES))) src/main.o

DLL_FILES = $(wildcard $(DLL_DIR)/*.cpp) src/dll.cpp
DLL_OBJS  = $(patsubst $(DLL_DIR)/%.cpp, $(DLL_DIR)/%.o, $(filter-out src/dll.cpp, $(DLL_FILES))) src/dll.o

IMGUI_FILES = $(wildcard src/dll/imgui/*.cpp)
IMGUI_OBJS  = $(patsubst src/dll/imgui/%.cpp, src/dll/imgui/%.o, $(IMGUI_FILES))

.PHONY: out

### Major processes
all: $(EXE_OUT) $(DLL_OUT) out

out:
	cp /mingw32/bin/libwinpthread-1.dll out/libwinpthread-1.dll
	cp crescent/newlisp.dll  out/daybreak/newlisp.dll
	cp crescent/newlisp.exe  out/daybreak/newlisp.exe
	cp crescent/server.lsp   out/daybreak/server.lsp
	cp crescent/client.lsp   out/daybreak/client.lsp
	cp crescent/autorun.nut  out/daybreak/autorun.nut
	./crescent/newlisp.exe -x crescent/client.lsp out/daybreak/client.exe
	cp -r out/daybreak mbaa/daybreak
	cp -r out/daybreak.exe mbaa/daybreak.exe

test:
	cd mbaa && ./daybreak.exe #./mbaa/daybreak/client.exe

clean:
	rm -rf mbaa/daybreak mbaa/daybreak.exe
	rm -rf out $(SRC_DIR)/*.o $(DLL_DIR)/*.o $(EXE_DIR)/*.o $(DLL_DIR)/imgui/*.o

### Compilation and linking
$(EXE_OUT): $(SRC_OBJS)
	mkdir -p out
	mkdir -p out/daybreak
	$(CXX) $^ -o $@ $(EXE_FLAGS)

$(DLL_OUT): $(DLL_OBJS)  $(IMGUI_OBJS)
	$(CXX) $^ -o $@ $(DLL_FLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp $(EXE_DIR)/%.cpp $(DLL_DIR)/%.cpp $(IMGUI_FILES)
	$(CXX) -c $(CPPFLAGS) $< -o $@
