#include "server.hpp"

NewLispServer::NewLispServer() {
  newlisp_handle = NULL;
  newlisp_eval = NULL;
}

NewLispServer::~NewLispServer() {
  FreeLibrary(newlisp_handle);
}

void NewLispServer::init(LPCWSTR dll_path) {
  if (!newlisp_handle) newlisp_handle = LoadLibraryW(dll_path);
  if (!newlisp_handle) gui_error("Load: could not load newlisp.dll!");
  if (!newlisp_eval)   newlisp_eval = (newlisp_eval_t)GetProcAddress(newlisp_handle, "newlispEvalStr");
  if (!newlisp_eval)   gui_error("Cannot initialize newlispEvalStr!");
}

char *NewLispServer::evaluate(std::string code) {
  char *res = newlisp_eval(code.c_str());
  return res;
}

char *NewLispServer::load_file(std::string path) {
  char *res = evaluate("(load \"" + path + "\")");
  return res;
}

void NewLispServer::draw_menu() {
  ImGui::Begin("Lisp", nullptr, ImGuiWindowFlags_NoTitleBar);
  ImGui::Text("Testing Lisp Menu");
  ImGui::End();
}

bool NewLispServer::stop_server() {
  newlisp_eval("(stop-server)");
  return strcmp("0", newlisp_eval("*server-status*")) == 0;
}
