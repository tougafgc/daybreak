/**
 * crescent_stdlib.cpp
 * By: tougafgc
 * Date: 1 December 2025
 *
 * All exported functions and "standard library" for
 * newLISP and Squirrel is contained here.
 */
#include "squirrel.hpp"

// All functions should be placed here, and then
// Squirrel wrappers after this block.
extern "C" {
  // This doesn't *really* need to
  // be externed to Lisp, but just as an example since
  // it's needed by Squirrel.
  void infobox(const char *text, const char *label) {
    MessageBoxA(NULL, text, label, MB_OK);
  }
}

// MessageBoxA wrapper
SQInteger sq_messagebox(HSQUIRRELVM vm) {
  const char *text;
  const char *label;

  // Push arguments in order
  sq_getstring(vm, 2, &text);
  sq_getstring(vm, 3, &label);

  // Function call is ready here
  infobox(text, label);
  return 0;
}

// General function that will register any Squirrel wrapper function. "name"
// argument will be the function that a Squirrel script will recofnize.
void register_fx(HSQUIRRELVM vm, SQInteger (*fx)(HSQUIRRELVM), const char *name) {
  sq_pushroottable(vm);
  sq_pushstring(vm, name, -1);
  sq_newclosure(vm, fx, 0);
  sq_newslot(vm, -3, SQFalse);
  sq_pop(vm, 1);
}

// In contrast to the above functions, this is a class function, so that
// all the above code doesn't clog SquirrelController. Everything is
// contained within this file and future possible associates.
void SquirrelController::register_all(HSQUIRRELVM vm) {
  register_fx(vm, sq_messagebox, "MessageBox");
}
