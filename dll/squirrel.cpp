/**
 * squirrel.cpp
 * By: tougafgc
 * Date: 1 December 2025
 *
 * All Squirrel related processes are implemented here.
 */
#include "squirrel.hpp"

// DO NOT initialize any class, module, or any heavy thing
// in the constructor. Use init() instead.
//
// TODO: make global variable in dll.cpp into a pointer and
// set inside the sq_wrapper thread?
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

  // TODO make this also initialize in all overloaded versions.
  script_running_p = false;
  selected_script = "";
  pwd = "";
  search_buf[0] = 0;
  char buffer[MAX_PATH];
  GetCurrentDirectoryA(MAX_PATH, buffer);
  pwd = buffer;
  refresh_files();
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

void SquirrelController::toggle_menu() {
  show_menu_p = !show_menu_p;
}

void SquirrelController::toggle_menu(bool state) {
  show_menu_p = state;
}

bool SquirrelController::show_menu() {
  return show_menu_p;
}

bool SquirrelController::script_active() const {
  return script_running_p;
}

const std::string& SquirrelController::get_script() const {
  return selected_script;
}

void SquirrelController::append_status(const std::string& message) {
  curr_status += timestamp() + message + "\n";
}

// Get current timestamp for status messages
std::string SquirrelController::timestamp() {
  auto now = std::time(nullptr);
  auto tm = *std::localtime(&now);
  std::ostringstream oss;

  oss << "[" << std::put_time(&tm, "%H:%M:%S") << "] ";
  return oss.str();
}

void SquirrelController::refresh_files() {
  files.clear();

  try {
    for (const auto& entry : std::filesystem::directory_iterator(pwd)) {
      if (entry.is_directory()) {
        files.push_back(FileEntry(entry.path().filename().string(), true));
      }
      else if (entry.is_regular_file()) {
        std::string extension = entry.path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        if (extension == ".nut" || extension == ".sq") {
          files.push_back(FileEntry(entry.path().filename().string(), false));
        }
      }
    }

    std::sort(files.begin(), files.end());
  }
  catch (const std::exception& e) {
    append_status("Error scanning directory: " + std::string(e.what()));
  }
}

bool SquirrelController::load_script(const std::string& filename) {
  try {
    std::string fullPath = pwd + "\\" + filename;
    std::ifstream file(fullPath);

    if (!file.is_open()) {
      append_status("Failed to open file: " + fullPath);
      return false;
    } else {
      append_status("Script loaded successfully: " + filename);
      return true;
    }
  }
  catch (const std::exception& e) {
    append_status("Error loading script: " + std::string(e.what()));
    return false;
  }
}

void SquirrelController::change_dir(const std::string& dir_name) {
  try {
    std::string new_dir;

    if (dir_name == "..") {
      std::filesystem::path path(pwd);

      if (path.has_parent_path()) new_dir = path.parent_path().string();
      else return; // Already at root
    }
    else {
      new_dir = pwd + "\\" + dir_name;
    }

    if (std::filesystem::is_directory(new_dir)) {
      pwd = new_dir;
      selected_script = "";
      refresh_files();
    }
  }
  catch (const std::exception& e) {
    append_status("Error navigating to directory: " + std::string(e.what()));
  }
}

void SquirrelController::draw_menu() {
  ImGui::Begin("Squirrel Script Loader", nullptr);

  ImGui::Text("Current Directory: %s", pwd.c_str());
  ImGui::SetNextItemWidth(-1);
  ImGui::InputTextWithHint("##search", "Filter results...", search_buf, sizeof(search_buf));

  ImGui::Columns(2, "script_loader_columns", true);
  ImGui::BeginChild("FileList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 3), true);

  // Add ".." as the first item in the file list
  if (ImGui::Selectable("[DIR] ..", false)) {
    change_dir("..");
  }

  std::string searchStr = search_buf;
  std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

  for (const auto& entry : files) {
    std::string display_name = entry.isDirectory ? "[DIR] " + entry.name : entry.name;
    std::string lower_name = display_name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    if (searchStr.empty() || lower_name.find(searchStr) != std::string::npos) {
      bool isSelected = !entry.isDirectory && (selected_script == entry.name);

      if (ImGui::Selectable(display_name.c_str(), isSelected)) {
        if (entry.isDirectory) change_dir(entry.name);
        else {
          selected_script = entry.name;
        }
      }

      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        if (entry.isDirectory) {
          change_dir(entry.name);
        }
        else if (load_script(entry.name)) {
          script_running_p = true;
          compile_file(vm, entry.name.c_str());
          append_status("Started script: " + entry.name);
          call("main");
        }
      }
    }
  }
  ImGui::EndChild();

  // Control buttons
  bool canStart = !selected_script.empty() && !script_running_p;
  bool can_stop_p = script_running_p;

  if (!canStart)
    ImGui::BeginDisabled();
  if (ImGui::Button("Start", ImVec2(-1, 0))) {
    if (load_script(selected_script)) {
      script_running_p = true;
      append_status("Started script: " + selected_script);
      call("main");
    }
  }
  if (!canStart)
    ImGui::EndDisabled();

  if (ImGui::Button("Reload", ImVec2(-1, 0))) {
    if (!selected_script.empty()) {
      append_status("Stopping script: " + selected_script);
      script_running_p = false;
      compile_file(vm, selected_script.c_str());
      append_status("Reloading script: " + selected_script);
      load_script(selected_script);
    }
    else {
      refresh_files();
      append_status("Refreshed file list");
    }
  }

  if (!can_stop_p) {
    ImGui::BeginDisabled();
    if (ImGui::Button("Stop", ImVec2(-1, 0))) {
      script_running_p = false;
      append_status("Stopped script: " + selected_script);
    }
    ImGui::EndDisabled();
  }

  ImGui::NextColumn();
  ImGui::Text("Status:");
  ImGui::BeginChild("StatusOutput", ImVec2(0, 0), true);
  ImGui::TextWrapped("%s", curr_status.c_str());

  // Auto-scroll to bottom when status changes
  static float len_status = 0;
  if (curr_status.length() != len_status) {
    ImGui::SetScrollHereY(1.0f);
    len_status = curr_status.length();
  }

  ImGui::EndChild();
  ImGui::Columns(1);
  ImGui::End();
}
