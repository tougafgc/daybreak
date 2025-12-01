/**
 * lisp.cpp
 * By: tougafgc
 * Date: 1 December 2025
 *
 * Implementation of all Lisp processes.
 **/
#include "lisp.hpp"

LispController::LispController() {
  newlisp_handle = NULL;
  newlisp_eval = NULL;
  show_menu_p = false; 
}

LispController::~LispController() {
  FreeLibrary(newlisp_handle);
}

void LispController::init(LPCWSTR dll_path) {
  if (!newlisp_handle) newlisp_handle = LoadLibraryW(dll_path);
  if (!newlisp_handle) gui_error("Load: could not load newlisp.dll!");
  if (!newlisp_eval)   newlisp_eval = (newlisp_eval_t)GetProcAddress(newlisp_handle, "newlispEvalStr");
  if (!newlisp_eval)   gui_error("Cannot initialize newlispEvalStr!");
}

char *LispController::evaluate(std::string code) {
  char *res = newlisp_eval(code.c_str());
  return res;
}

char *LispController::load_file(std::string path) {
  char *res = evaluate("(load \"" + path + "\")");
  return res;
}

void LispController::draw_menu() {
  ImGui::Begin("Lisp", nullptr, ImGuiWindowFlags_NoTitleBar);
  ImGui::Text("Testing Lisp Menu");
  ImGui::End();
}

void LispController::toggle_menu() {
  show_menu_p = !show_menu_p;
}

void LispController::toggle_menu(bool state) {
  show_menu_p = state;
}

bool LispController::show_menu() {
  return show_menu_p;
}

bool LispController::start_server() {
  newlisp_eval("(start-server)");
  return strcmp("true", newlisp_eval("*server-status*")) == 0; 
}

bool LispController::stop_server() {
  newlisp_eval("(stop-server)");
  return strcmp("nil", newlisp_eval("*server-status*")) == 0;
}
