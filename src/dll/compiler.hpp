#ifndef COMPILER_HPP_
#define COMPILER_HPP_

#include "../lib.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

#include "squirrel3/include/squirrel.h"
#include "squirrel3/include/sqstdsystem.h"

typedef SQInteger (*SQLEXREADFUNC)(SQUserPointer userdata);

class SquirrelCompiler {
  public:
    int DEFAULT_STACK = 1024;

    SquirrelCompiler();
    ~SquirrelCompiler();

    void init(const char *path);
    void init(int stack_size, const char *path);
    void draw_menu();
    void call(const char *func);

  private:
    HSQUIRRELVM vm;
    int stack_size;
};

#endif // COMPILER_HPP_
