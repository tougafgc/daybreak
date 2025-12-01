#include "compiler.hpp"

SQInteger file_readASCII(SQUserPointer file) {
  int ret;
  char c;
  if ((ret = fread(&c, sizeof(c), 1, (FILE *)file)) > 0) {
    return c;
  }

  return 0;
}

int compile_file(HSQUIRRELVM vm, const char *filename) {
  FILE *f = fopen(filename, "rb");
  if (f) {
    sq_compile(vm, file_readASCII, f, filename, 1);
    fclose(f);
  } else {
    return 0;
  }

  sq_pushroottable(vm);

  // Any top-level squirrel code will run here
  if (SQ_FAILED(sq_call(vm, 1, SQFalse, SQTrue))) {
    gui_error("Cannot load file!");
    return 0;
  }

  return 1;
}

// TODO make this general so functions
// inside can also be extern C'd to be
// available in Lisp
SQInteger sq_messagebox(HSQUIRRELVM vm) {
  const char *text;
  const char *label;

  sq_getstring(vm, 2, &text);
  sq_getstring(vm, 3, &label);

  // wrap this in something extern C-able
  MessageBoxA(NULL, text, label, MB_OK);
  return 0;
}

void register_fx(HSQUIRRELVM vm, SQInteger (*fx)(HSQUIRRELVM), const char *name) {
  sq_pushroottable(vm);
  sq_pushstring(vm, name, -1);
  sq_newclosure(vm, fx, 0);
  sq_newslot(vm, -3, SQFalse);
  sq_pop(vm, 1);
}

void register_all(HSQUIRRELVM vm) {
  register_fx(vm, sq_messagebox, "MessageBox");
}

SquirrelCompiler::SquirrelCompiler() {
  this->stack_size = 0;
}

void SquirrelCompiler::init(const char *path) {
  vm = sq_open(SquirrelCompiler::DEFAULT_STACK);
  register_all(vm);

  if (!compile_file(vm, path)) {
    gui_error("Unable to compile " + std::string(path) + "!");
  }
}

void SquirrelCompiler::init(int stack_size, const char *path) {
  vm = sq_open(stack_size);
  register_all(vm);

  if (!compile_file(vm, path)) {
    gui_error("Unable to compile " + std::string(path) + "!");
  }
}

SquirrelCompiler::~SquirrelCompiler() {
  sq_close(vm);
}

void SquirrelCompiler::call(const char *func) {
  sq_pushroottable(vm);
  sq_pushstring(vm, func, -1);

  if (SQ_FAILED(sq_get(vm, -2))) {
    sq_pop(vm, 1);
    gui_error("Not found, autorun.nut::" + std::string(func) + "!");
    return;
  }

  sq_pushroottable(vm);

  if (SQ_FAILED(sq_call(vm, 1, SQFalse, SQTrue))) {
    gui_error("Cannot run, autorun.nut::" + std::string(func) + "!");
    return;
  }

  sq_pop(vm, 1);
}

void SquirrelCompiler::draw_menu() {
  ImGui::Begin("Squirrel", nullptr, ImGuiWindowFlags_NoTitleBar);
  ImGui::Text("Testing Squirrel Menu");
  ImGui::End();
}
