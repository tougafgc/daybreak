#include "gamepad.hpp"

BOOL CALLBACK callback(const DIDEVICEINSTANCE *dinst, VOID *context) {
  std::vector<Userpad> *connected = static_cast<std::vector<Userpad> *>(context);
  int type = GET_DIDEVICE_TYPE(dinst->dwDevType);

  if (type == DI8DEVTYPE_KEYBOARD || type == DI8DEVTYPE_JOYSTICK ||
      type == DI8DEVTYPE_GAMEPAD) {// || DIDEVTYPE_HID) {
      Userpad pad(dinst->guidInstance, dinst->tszInstanceName, (type == DI8DEVTYPE_KEYBOARD) ?
                                                               ControllerType::KEYBOARD :
                                                               ControllerType::JOYSTICK);
    connected->push_back(pad);
  }

  return DIENUM_CONTINUE;
}

void GamepadController::add_controller(Userpad up) {
  connected.push_back(up);
}

void GamepadController::update_connections() {
  if (!dinput) return;

  std::vector<Userpad> snap;
  int res = dinput->EnumDevices(DI8DEVCLASS_ALL, callback, &snap, DIEDFL_ATTACHEDONLY);
  if (FAILED(res)) gui_error("Cannot enumerate devices!");

  // if found: !snap & connected, remove from connected.
  // Since the updated vector doesn't include anything that follows
  // the two rules, explicit removal doesn't need to happen.
  connected.erase(std::remove_if(connected.begin(), connected.end(),
                                 [&](const Userpad &pad) {
                                   return std::none_of(snap.begin(), snap.end(),
                                                       [&](const Userpad &other) {
                                                         return other == pad;
                                                       });
                                 }), connected.end());

  for (auto& item : snap) {
    auto it = std::find(connected.begin(), connected.end(), item);
    if (it == connected.end()) connected.push_back(item);
  }
}

GamepadController::GamepadController() {
  show_menu_p = false;
}

void GamepadController::init(HINSTANCE hinst) {
  int res = DirectInput8Create(hinst, DIRECTINPUT_VERSION, IID_IDirectInput8A, (void **)&dinput, nullptr);
  if (FAILED(res)) gui_error("Cannot create DI8 controller.");

  connected.clear();
  update_connections();

  left_free = true;
  right_free = true;
}

void GamepadController::fsm(Userpad &pad) {
  std::string str = std::to_string(pad.keys.at(KEY_UP).pressed()) + " " + std::to_string(pad.keys.at(KEY_UP).pressed_p);

  // middle -> right edge
  // middle -> left edge
  if (pad.side == PlayerSide::MIDDLE) {
    if (pad.keys.at(KEY_LEFT).pressed() && left_free) {
      pad.set_addresses(PlayerSide::LEFT);
      pad.side = PlayerSide::LEFT;
      left_free = false;
    } else if (pad.keys.at(KEY_RIGHT).pressed() && right_free) {
      pad.set_addresses(PlayerSide::RIGHT);
      pad.side = PlayerSide::RIGHT;
      right_free = false;
    }
  }
  // left -> middle edge
  else if (pad.side == PlayerSide::LEFT) {
    if (pad.keys.at(KEY_RIGHT).pressed()) {
      pad.set_addresses(PlayerSide::MIDDLE);
      pad.side = PlayerSide::MIDDLE;
      left_free = true;
    }
  }
  // right -> middle edge
  else if (pad.side == PlayerSide::RIGHT) {
    if (pad.keys.at(KEY_LEFT).pressed()) {
      pad.set_addresses(PlayerSide::MIDDLE);
      pad.side = PlayerSide::MIDDLE;
      right_free = true;
    }
  }

  // left & right side menu operations
  if (pad.side != PlayerSide::MIDDLE) {
    if (pad.state == State::MAPPING) {
      if (pad.keys.at(KEY_UP).pressed()) {
        if (pad.idx == -1) pad.state = State::NONE;
        else if (pad.idx == 0 || (pad.type == ControllerType::JOYSTICK && pad.idx == 4)) {
          pad.state = State::NONE;
          pad.idx = -1;
        } else if (pad.idx > 0) pad.idx--;
      }

      else if (pad.keys.at(KEY_DOWN).pressed()) {
        if (pad.idx == -1 && pad.type == ControllerType::JOYSTICK) pad.idx = 4;
        else if (pad.idx < 12) pad.idx++;
      }
    }

    else if (pad.state == State::NONE) {
      if (pad.keys.at(KEY_DOWN).pressed() && pad.idx == -1) {
        pad.state = State::MAPPING;
      }

      //else if (pad.keys.at(KEY_UP) && pad.idx == -1) {
      // testing/mapping menu
      //}

      else if (pad.keys.at(KEY_RIGHT).pressed() && pad.side == PlayerSide::LEFT) {
        left_free = true;
        pad.side = PlayerSide::MIDDLE;
      }

      else if (pad.keys.at(KEY_LEFT).pressed() && pad.side == PlayerSide::RIGHT) {
        right_free = true;
        pad.side = PlayerSide::MIDDLE;
      }
    }
  }
}

void GamepadController::draw_side(std::optional<Userpad *> pad, int offset) {
  Userpad *side = *pad;

  //ImGui::SameLine(offset);

  std::regex pattern("Controller \\((.*)\\)");
  std::smatch name_match;

  if (std::regex_search(side->name, name_match, pattern)) {
    ImGui::Text(name_match[1].str().c_str());
  } else {
    ImGui::Text(side->name.c_str());
  }

  if (side->state == State::MAPPING) {
    std::vector<std::string> options = {"Up:", "Down:", "Left:", "Right:", "A:", "B:", "C:", "D:", "E:", "Start:", "FN1:", "FN2:", "A+B:"};

    if (side->side == PlayerSide::RIGHT) offset -= 150;
    for (unsigned int i = (side->type == ControllerType::KEYBOARD) ? 0 : 4; i < options.size(); i++) {
      std::string row = options.at(i) + " ";
      row += (side->keys.at(i).mapped == -1) ? "Unset..." : std::to_string(side->keys.at(i).mapped);
      ImGui::Selectable(row.c_str(), (int)i == side->idx, 0, ImVec2(0.f, 0.f));
    }

  } else if (side->state == State::TESTING) {
    ImGui::Text("Testing");
  }
}

void GamepadController::draw_menu() {
  std::optional<Userpad *> left_pad = std::nullopt;
  std::optional<Userpad *> right_pad = std::nullopt;

  ImGui::Begin("Controller Menu", nullptr);

  update_connections();
  if (connected.size() == 0) {
    ImGui::Text("No devices connected...");
  } else {
    int middle_amnt = 0;
    int left_amnt = 0;
    int right_amnt = 0;

    for (auto& pad : connected) {
      if (pad.side == PlayerSide::LEFT) {
        left_amnt++;
        left_pad = &pad;
      } else if (pad.side == PlayerSide::MIDDLE) {
        middle_amnt++;
        std::string menuItem = "";

        std::regex pattern("Controller \\((.*)\\)");
        std::smatch name_match;
        if (std::regex_search(pad.name, name_match, pattern)) {
          menuItem = name_match[1].str().c_str();
        } else {
          menuItem = pad.name.c_str();
        }

        ImGui::SameLine((ImGui::GetWindowSize().x - ImGui::CalcTextSize(menuItem.c_str()).x) * 0.5f);
        ImGui::Text(menuItem.c_str());
        if (left_amnt + middle_amnt + right_amnt != (int)connected.size()) ImGui::NewLine();
      } else if (pad.side == PlayerSide::RIGHT) {
        right_amnt++;
        right_pad = &pad;
      }

      fsm(pad);
    }

    ImGui::Dummy(ImVec2(0.0f, 5.f));
    ImGui::Separator();

    ImGui::Dummy(ImVec2(0.0f, 3.f));
    ImGui::Spacing();

    ImGuiStyle& style = ImGui::GetStyle();
    float spacing = style.ItemSpacing.x;
    float horiz = (ImGui::GetContentRegionAvail().x - spacing) * 0.5f;

    ImGui::BeginChild("left pane", ImVec2(horiz, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
    if (left_pad.has_value()) draw_side(left_pad, 0);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("right pane", ImVec2(horiz - 10, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
    if (right_pad.has_value()) draw_side(right_pad, ImGui::GetWindowSize().x - ImGui::CalcTextSize((*right_pad)->name.c_str()).x - 20);
    ImGui::EndChild();
  }

  ImGui::End();
}

void GamepadController::toggle_menu() {
  show_menu_p = !show_menu_p;
}

void GamepadController::toggle_menu(bool state) {
  show_menu_p = state;
}

bool GamepadController::show_menu() {
  return show_menu_p;
}
