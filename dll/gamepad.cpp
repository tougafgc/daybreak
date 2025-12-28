#include "gamepad.hpp"

BOOL CALLBACK callback(const DIDEVICEINSTANCE *dinst, VOID *context) {
  GamepadController *gamepad = static_cast<GamepadController *>(context);

  switch (GET_DIDEVICE_TYPE(dinst->dwDevType)) {
    case DI8DEVTYPE_KEYBOARD:
    case DI8DEVTYPE_JOYSTICK:
    case DI8DEVTYPE_GAMEPAD:
      gamepad->add_controller({ dinst->guidInstance, dinst->tszInstanceName, PlayerSide::MIDDLE });
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
}

void GamepadController::draw_menu() {
  ImGui::Begin("Controller Menu", nullptr);

  // keyboard
  Userpad &kb = connected.at(0);

  if (kb.side == MIDDLE) {
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && !leftSide.has_value()) {
      kb.side = PlayerSide::LEFT;
      leftSide = kb;
    } else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && !rightSide.has_value()) {
      kb.side = PlayerSide::RIGHT;
      rightSide = kb;
    }
  } else {
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
      if (connected.at(0).side == PlayerSide::LEFT) {
        leftSide = std::nullopt;
      } else if (connected.at(0).side == PlayerSide::RIGHT) {
        rightSide = std::nullopt;
      }

      connected.at(0).side = PlayerSide::MIDDLE;
    } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
      // controller binding menu
    }
  }

  // process controllers here

  if (connected.size() == 0) {
    ImGui::Text("No devices connected...");
  }
  else {
    int middle_amnt = 0;
    for (unsigned int i = 0; i < connected.size(); i++) {
      Userpad curr = connected.at(i);
      if (curr.side != MIDDLE) {
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
    if (leftSide.has_value()) {
      ImGui::SameLine(0);
      ImGui::Text(leftSide->name.c_str());
    }

    if (rightSide.has_value()) {
      ImGui::SameLine(ImGui::GetWindowSize().x - ImGui::CalcTextSize(rightSide->name.c_str()).x - 10);
      ImGui::Text(rightSide->name.c_str());
    }
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
