/**
 * squirrel.cpp
 * By: tougafgc
 * Date: 1 December 2025
 *
 * All Squirrel related processes are implemented here.
 */
#include "squirrel.hpp"

SquirrelController::SquirrelController() {
  this->stack_size = 0;
}

SquirrelController::~SquirrelController() {
  sq_close(vm);
}

void SquirrelController::init(const char *path) {
  vm = sq_open(SquirrelController::DEFAULT_STACK);
  register_all(vm);

  if (!compile_file(vm, path)) {
    gui_error("Unable to compile " + std::string(path) + "!");
  }
}

void SquirrelController::init(int stack_size, const char *path) {
  vm = sq_open(stack_size);
  register_all(vm);

  if (!compile_file(vm, path)) {
    gui_error("Unable to compile " + std::string(path) + "!");
  }
}

// Reader function used by compile_file
SQInteger file_readASCII(SQUserPointer file) {
  int ret;
  char c;
  if ((ret = fread(&c, sizeof(c), 1, (FILE *)file)) > 0) {
    return c;
  }

  return 0;
}

int SquirrelController::compile_file(HSQUIRRELVM vm, const char *filename) {
  this->filename = std::string(filename);

  FILE *f = fopen(filename, "rb");
  if (f) {
    sq_compile(vm, file_readASCII, f, filename, 1);
    fclose(f);
  } else {
    return 0;
  }

  sq_pushroottable(vm);

  // NOTE: Any top-level squirrel code will run here
  if (SQ_FAILED(sq_call(vm, 1, SQFalse, SQTrue))) {
    gui_error("Cannot load file!");
    return 0;
  }

  return 1;
}

void SquirrelController::call(const char *func) {
  sq_pushroottable(vm);
  sq_pushstring(vm, func, -1);

  if (SQ_FAILED(sq_get(vm, -2))) {
    sq_pop(vm, 1);
    gui_error("Not found, " + filename + "::" + std::string(func) + "!");
    return;
  }

  sq_pushroottable(vm);

  if (SQ_FAILED(sq_call(vm, 1, SQFalse, SQTrue))) {
    gui_error("Not found, " + filename + "::" + std::string(func) + "!");
    return;
  }

  sq_pop(vm, 1);
}

void SquirrelController::draw_menu() {
  ImGui::Begin("Squirrel", nullptr, ImGuiWindowFlags_NoTitleBar);
  ImGui::Text("Testing Squirrel Menu");
  ImGui::End();
}

void SquirrelController::toggle_menu() {
  show_menu_p = !show_menu_p;
}

void SquirrelController::toggle_menu(bool state) {
  show_menu_p = state;
}

bool SquirrelController::show_menu() {
  return show_menu_p;
}
