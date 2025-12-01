/**
 * lisp.hpp
 * By: tougafgc
 * Date: 1 December 2025
 *
 * All Lisp oriented processes are contained here, both 
 * creating the REPL, Dear ImGui menu, and any intermediate
 * tasks.
 **/
#ifndef LISP_HPP_
#define LISP_HPP_

#include "../lib/lib.hpp"
#include "../lib/imgui/imgui.h"
#include "../lib/imgui/imgui_impl_dx9.h"
#include "../lib/imgui/imgui_impl_win32.h"

typedef char* (__stdcall *newlisp_eval_t)(const char *code);

class LispController {
  public:
    LispController();
    ~LispController();

    // init() exists because calling initialization code
    // in the main DLL thread will cause the hook to crash
    // on startup.
    void init(LPCWSTR dll_path);

    // Evaluates a single line of newLISP code, and it
    // also preserves state.
    char *evaluate(std::string code);

    // Load a file so that functions (or top-level code)
    // will be available to the REPL. Shortcut for 
    // this->evaluate("(load \"file_path\" "));
    char *load_file(std::string path);

    // Draw a Dear ImGui menu
    void draw_menu();

    // Toggle the Dear ImGui menu
    void toggle_menu();

    // Force a show/hide state
    void toggle_menu(bool state);

    // Get the value to determine if the Dear ImGui menu should be shown
    bool show_menu();

    // Start a newLISP REPL server that can be connected to 
    // by client.exe/client.lsp
    // TODO: integrate into Dear ImGui
    bool start_server();

    // Stop a running newLISP REPL server, if running.
    bool stop_server();

  private:
    // Handle for newlisp.dll to extract functions.
    HMODULE newlisp_handle;

    // Function pointer for newLispEvalStr.
    newlisp_eval_t newlisp_eval;

    // Toggle switch for whether the Dear ImGui
    // menu should be drawn.
    bool show_menu_p;

};

#endif // LISP_HPP_
