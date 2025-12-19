# Modifying and Extending Daybreak

## Preface

This project is dependent only on MinGW32. No UCRT or MSVC binaries or tools are used in the
development or compilation of Daybreak. Installing [MSYS2](https://www.msys2.org/)
and the x86 gcc/g++ toolchain are the only requirements to get it working properly. 

The newLISP server provided in Daybreak listens to `localhost:25565`, and is currently hardcoded to listen here. 
Telnet works, but it does not support the pretty prompt that `server.lsp` provides. `client.lsp` and its compiled 
cohort `client.exe` will automatically look for this address and connect to it.

Daybreak looks for autorun.nut with functions draw() and main() to run Squirrel. Currently (as of 18 December 2025),
Daybreak will load this file on startup, and will execute main() once in its own thread, and will execute draw()
every time D3DX9->Present() is called. See `dll.cpp::sq_wrapper` and `dll.cpp::hook_present`.

## Compiliation

*Please compile using MinGW32.*
1. Download this repository
2. `./download_deps.sh` to download MBAACC and newLISP
3. `make first` to create Squirrel libraries and prepare Dear ImGui headers/sources (run `make all` on subsequent compilations)
4. `make test` to launch Daybreak

## Styleguide

### General
* Indent with 2 spaces
* Pointers and references are attached to the argument (void *ptr, int &ref)
* Function returns are attached to the type (void* fx1, int& fx2)
* Use std::string unless char* or char[] is specifically needed
* Please avoid `using namespace ...;`

### Naming Conventions (C++)
* Class and Struct names are PascalCase
* Variables and method names are always snake_case
* Any flag, predicate, or true/false should have "_p" added to the end of the name (running\_p, show\_menu\_p, etc)
* Any hooked function for D3DX9 should have the type name also be the name of the original function, and the stored callback being `orig_(fx_name)_fx`. D3DX9::EndScene would look like `EndScene_t orig_endscene_fx`.
* Any hooked windows callback should have the name `hook_windows_fx_name`. WndProc -> hook_wnd\_proc.
* Global variables should have "g_" at the front of its name.
* Any Squirrel wrapper function should have "sq_" at the front of its name.
* Registered names in the Squirrel VM for functions should follow snake_case unless it is directly from Windows, which it should be PascalCase.

### Naming Conventions (Lisp)
* Global variables should use \*earmuffs\*.
* word breaks should use dashes, unless they're from FFI.

### File Naming
| Type | Extension |
|------|-----------|
| C++ source | .cpp |
| C++ header | .hpp |
| newLISP    | .lsp |
| Squirrel (using Crescent) | .crsc |
| Squirrel (vanilla)        | .nut  |
