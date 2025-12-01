# Melty Blood: Daybreak

Daybreak aims at taking the many individual utilites for Melty Blood: AACC and making them
available altogether inside the game's base executable. In addition, an interactive scripting
environment is provided by the newLISP and Squirrel languages for easy extensibility without the need
for recompilation.

[Play Melty Blood!](https://play.meltyblood.club/) | [newLISP](http://www.newlisp.org) | [Squirrel](http://squirrel-lang.org/)

## Installation

### From Source

*Please compile using MinGW32.*
1. Download this repository
2. `./download_deps.sh` to download MBAACC and newLISP
3. `make first` to create Squirrel libraries and prepare Dear ImGui headers/sources (run `make all` on subsequent compilations)
4. `make test` to launch Daybreak

### From Releases Page

Download `daybreak_vX.X.X.zip` and unzip it inside of a pre-existing MBAA folder. `daybreak.exe` and `lipwinpthread-1.dll` 
should be in the same folder as `MBAA.exe`, and everything else should be inside the `daybreak/` folder.

## Usage

Either by double-clicking `daybreak.exe`, running it in the command line, or typing `make test` 
MBAACC will automatically open with all modules preloaded.

## Hacking & Modification

This project is dependent only on MinGW32. No UCRT or MSVC binaries or tools are used in the
development or compilation of Daybreak. Installing [MSYS2](https://www.msys2.org/)
and the x86 gcc/g++ toolchain are the only requirements to get it working properly. 

The newLISP server provided in Daybreak listens to `localhost:25565`, and is currently hardcoded to listen here. 
Telnet works, but it does not support the pretty prompt that `server.lsp` provides. `client.lsp` and its compiled 
cohort `client.exe` will automatically look for this address and connect to it.

Daybreak looks for autorun.nut with functions draw() and main() to run Squirrel. Currently (as of 1 December 2025),
Daybreak will load this file on startup, and will execute main() once in its own thread, and will execute draw()
every time D3DX9->Present() is called. See `dll.cpp::sq_wrapper` and `dll.cpp::hkPresent`.

## Extending Crescent STDLIB

"Crescent" is the name for the collection of Lisp/Squirrel files and functions that are used while an instance
of Melty Blood is running. Currently, the standard library, or the set of FFI functions that are defined in C++
and exported to Lisp/Squirrel, is quite small. `dll/crescent_stdlib.cpp` outlines how functions can be exported to 
Lisp, and how to wrap them in a way that Squirrel scripts will recognize them.

## Roadmap

- Write text inside of Melty
- Contain the newLISP client inside of Melty (open/closing by pressing a button combination)
- Controller mappings
- button test menu
- netplay with GGPO
