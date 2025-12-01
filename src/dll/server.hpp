#ifndef SERVER_HPP_
#define SERVER_HPP_

#include "../lib.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

typedef char* (__stdcall *newlisp_eval_t)(const char *code);

class NewLispServer {
  public:
    NewLispServer();
    ~NewLispServer();

    void init(LPCWSTR dll_path);

    char *evaluate(std::string code);
    char *load_file(std::string code);
    void draw_menu();
    bool stop_server();

  private:
    HMODULE newlisp_handle;
    newlisp_eval_t newlisp_eval;
};


#endif // SERVER_HPP_
