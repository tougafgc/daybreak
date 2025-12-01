/**
 * squirrel.hpp
 * By: tougafgc
 * Date: 1 December 2025
 *
 * All Squirrel oriented processes are contained here,
 * Dear ImGui menu, and any intermediate tasks.
 **/
#ifndef SQUIRREL_HPP_
#define SQUIRREL_HPP_

#include "../lib/lib.hpp"
#include "../lib/imgui/imgui.h"
#include "../lib/imgui/imgui_impl_dx9.h"
#include "../lib/imgui/imgui_impl_win32.h"

#include "../lib/squirrel/include/squirrel.h"
#include "../lib/squirrel/include/sqstdsystem.h"

typedef SQInteger (*SQLEXREADFUNC)(SQUserPointer userdata);

class SquirrelController {
  public:
    static const int DEFAULT_STACK = 1024;

    SquirrelController();
    ~SquirrelController();

    // Since calling class/library functions in the DLL's main
    // thread will crash the program, a separate initialization
    // function is needed. This creates a Squirrel VM with
    // 1024 stack slots, and the path to the filename.
    // Use Windows conventions for the path (\\ instead of /).
    void init(const char *path);

    // Above, but `stack_size' is used in place of 1024 slots.
    void init(int stack_size, const char *path);

    // Call a function loaded from a .nut script.
    void call(const char *func);

    // Draw a Dear ImGui menu
    void draw_menu();

    // Toggle the Dear ImGui menu
    void toggle_menu();

    // Force a show/hide state
    void toggle_menu(bool state);

    // Get the value to determine if the Dear ImGui menu should be shown
    bool show_menu();

  private:
    HSQUIRRELVM vm;
    int stack_size;
    bool show_menu_p;
    std::string filename;

  // Compile a given file to use its functions
    // in MBAA's address space.
    SQInteger compile_file(HSQUIRRELVM vm, const char *filename);

    // Register all wrapped functions in crescent_stdlib.cpp
    // to be available in a loaded Squirrel script.
    void register_all(HSQUIRRELVM vm);
};

#endif // SQUIRREL_HPP_
