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

  buffer = "crescent> ";
  autoscroll = true;
  pos_imgui = buffer.length();
  strcpy(input_buf, buffer.c_str());
}

char *LispController::evaluate(std::string code) {
  char *res = newlisp_eval(code.c_str());
  return res;
}

char *LispController::load_file(std::string path) {
  char *res = evaluate("(load \"" + path + "\")");
  return res;
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

void LispController::eval_imgui(const std::string& cmd) {
  if (cmd.empty()) {
    buffer += "\ncrescent> ";
    return;
  } else if (cmd == "clear") {
    buffer = "crescent> ";
  } else {
    buffer += "\n" + std::string(evaluate(cmd)) + "\ncrescent> ";
  }

  pos_imgui = buffer.length();
  strcpy(input_buf, buffer.c_str());
  autoscroll = true;
}

int LispController::handle_input_imgui(ImGuiInputTextCallbackData* data) {
  buffer = data->Buf;

  // TODO fix pressing ctrl+enter to give a newline without evaluating the buffer
  if (data->EventKey == ImGuiKey_Enter) {
    if (ImGui::GetIO().KeyCtrl) {
      data->EventFlag = ImGuiInputTextFlags_CallbackAlways;
    } else {
      data->InsertChars(data->CursorPos, "\n");
    }

    return 1;
  }

  // Restrict cursor position to be at or after the last prompt
  size_t last_pos = buffer.rfind("> ");
  if (last_pos != std::string::npos) {
    int min_pos = last_pos + 2;
    if (data->CursorPos < min_pos)      data->CursorPos = min_pos;
    if (data->SelectionStart < min_pos) data->SelectionStart = min_pos;
    if (data->SelectionEnd < min_pos)   data->SelectionEnd = min_pos;
  }

  pos_imgui = data->CursorPos;
  return 0;
}

void LispController::draw_menu() {
  auto handle_callback = [](ImGuiInputTextCallbackData *data) -> int {
    LispController *obj = static_cast<LispController *>(data->UserData);
    return obj->handle_input_imgui(data);
  };
  int text_flags = ImGuiInputTextFlags_CallbackAlways   |
                   ImGuiInputTextFlags_EnterReturnsTrue |
                   ImGuiInputTextFlags_CtrlEnterForNewLine;

  ImGui::Begin("Lisp Menu", nullptr);
  ImGui::Text("Console (type at the prompt)");
  ImGui::SameLine();
  if (ImGui::Button("Clear")) {
    buffer = "> ";
    pos_imgui = buffer.length();
    strcpy(input_buf, buffer.c_str());
  }

  // Multiline config variables
  float scroll_region_hgt = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
  ImGui::BeginChild("ScrollingRegion", ImVec2(0, -scroll_region_hgt), false, ImGuiWindowFlags_HorizontalScrollbar);
  if (ImGui::InputTextMultiline("##console", input_buf, sizeof(input_buf), ImVec2(-FLT_MIN, -FLT_MIN),
        text_flags, handle_callback, this))
    {
      std::string text(input_buf);
      size_t last_pos = text.rfind("> ");

      if (last_pos != std::string::npos) {
        std::string cmd = text.substr(last_pos + 2);
        while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r')) cmd.pop_back();
        eval_imgui(cmd);
      }
    }

  if (autoscroll) {
    ImGui::SetScrollHereY(1.0f);
    autoscroll = false;
  }

  ImGui::EndChild();
  ImGui::End();
}
