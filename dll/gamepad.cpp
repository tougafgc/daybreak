#include "gamepad.hpp"

BOOL CALLBACK callback(const DIDEVICEINSTANCE *dinst, VOID *context) {
  GamepadController *gamepad = static_cast<GamepadController *>(context);

  switch (GET_DIDEVICE_TYPE(dinst->dwDevType)) {
    case DI8DEVTYPE_KEYBOARD:
    case DI8DEVTYPE_JOYSTICK:
    case DI8DEVTYPE_GAMEPAD:
      gamepad->add_controller({ dinst->guidInstance, dinst->tszInstanceName, PlayerSide::MIDDLE, State::NONE });
      break;
    default:
      break;
  }

  return DIENUM_CONTINUE;
}

void GamepadController::add_controller(Userpad up) {
  connected.push_back(up);
}

GamepadController::GamepadController() {
  show_menu_p = false;
}

void GamepadController::init(HINSTANCE hinst) {
  int res = DirectInput8Create(hinst, DIRECTINPUT_VERSION, IID_IDirectInput8A, (void **)&dinput, nullptr);
  if (FAILED(res)) gui_error("Cannot create DI8 controller.");

  connected.clear();
  res = dinput->EnumDevices(DI8DEVCLASS_ALL, callback, this, DIEDFL_ATTACHEDONLY);
  if (FAILED(res)) gui_error("Cannot enumerate devices!");

  left_free = true;
  right_free = true;
}

void GamepadController::fsm(Userpad &pad) {
  switch (pad.side) {
    case PlayerSide::MIDDLE:
      if (pad.pressed(KEY_LEFT) && left_free) {
        pad.side = PlayerSide::LEFT;
        left_free = false;
      } else if (pad.pressed(KEY_RIGHT) && right_free) {
        pad.side = PlayerSide::RIGHT;
        right_free = false;
      }
      break;
    case PlayerSide::LEFT:
    case PlayerSide::RIGHT:
      if (pad.state == State::MAPPING) {
        if (pad.pressed(KEY_UP)) {
          if (pad.idx == -1) pad.state = State::NONE;
          else if (pad.idx == 0) {
            pad.state = State::NONE;
            pad.idx--;
          } else if (pad.idx > 0) pad.idx--;
        }

        else if (pad.pressed(KEY_DOWN)) {
          if (pad.idx < 7) pad.idx++;
        }
      }

      else if (pad.state == State::NONE) {
        if (pad.pressed(KEY_DOWN) && pad.idx == -1) {
          pad.state = State::MAPPING;
        }

        //else if (pad.pressed(KEY_UP) && pad.idx == -1) {
          // testing/mapping menu
        //}

        else if (pad.pressed(KEY_RIGHT) && pad.side == PlayerSide::LEFT) {
          left_free = true;
          pad.side = PlayerSide::MIDDLE;
        }

        else if (pad.pressed(KEY_LEFT) && pad.side == PlayerSide::RIGHT) {
          right_free = true;
          pad.side = PlayerSide::MIDDLE;
        }

      }
      break;
    default:
      break;
  }
}

void GamepadController::draw_side(std::optional<Userpad *> pad, int offset) {
  Userpad *side = *pad;

  ImGui::SameLine(offset);
  ImGui::Text(side->name.c_str());

  if (side->state == State::MAPPING) {
    std::vector<std::string> options = {"A", "B", "C", "D", "E", "Start", "FN1", "FN2"};

    if (side->side == PlayerSide::RIGHT) offset -= 150;
    for (unsigned int i = 0; i < options.size(); i++) {
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
      ImGui::Selectable(options.at(i).c_str(), (int)i == side->idx, 0, ImVec2(200.f, 0.f));
    }

    ImGui::Text("%d", side->idx);

  } else if (side->state == State::TESTING) {
    ImGui::Text("Testing");
  }
}

void GamepadController::draw_menu() {
  std::optional<Userpad *> left_pad = std::nullopt;
  std::optional<Userpad *> right_pad = std::nullopt;

  ImGui::Begin("Controller Menu", nullptr);

  for (auto& pad : connected) {
    fsm(pad);
  }

  if (connected.size() == 0) {
    ImGui::Text("No devices connected...");
  } else {
    int middle_amnt = 0;
    for (unsigned int i = 0; i < connected.size(); i++) {
      Userpad curr = connected.at(i);
      if (curr.side != MIDDLE) {
        if (curr.side == PlayerSide::LEFT) left_pad = &curr;
        else if (curr.side == PlayerSide::RIGHT) right_pad = &curr;
        continue;
      }

      std::string menuItem = curr.name + " " + std::to_string(curr.side);
      ImGui::SameLine((ImGui::GetWindowSize().x - ImGui::CalcTextSize(menuItem.c_str()).x) * 0.5f);
      ImGui::Text(menuItem.c_str());
      middle_amnt++;
    }

    if (middle_amnt == 0) {
      ImGui::Text(" ");
    }

    ImGui::Separator();
    if (left_pad.has_value()) draw_side(left_pad, 0);
    if (right_pad.has_value()) draw_side(right_pad, ImGui::GetWindowSize().x - ImGui::CalcTextSize((*right_pad)->name.c_str()).x - 10);
    ImGui::End();
  }
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
