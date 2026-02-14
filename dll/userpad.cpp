#include "gamepad.hpp"

Userpad::Userpad(GUID id, const char *name, ControllerType type) {
  this->id = id;
  this->name = std::string(name);
  side = PlayerSide::MIDDLE;
  this->type = type;
  idx = -1;

  for (int key = KEY_UP; key <= KEY_FN2; key++) {
    keys.insert({ key, {false, false, -1, NULL, NULL} });
  }
}

void Userpad::save_previous_keys() {
  for (int key = KEY_UP; key <= KEY_FN2; key++) {
    Input *input = &keys.at(key);
    input->previous_p = input->pressed_p;
  }
}

bool Userpad::direction_neutral_p() {
  return (!keys.at(KEY_UP).pressed_p && !keys.at(KEY_DOWN).pressed_p &&
          !keys.at(KEY_LEFT).pressed_p && !keys.at(KEY_RIGHT).pressed_p);
}

uint8_t Userpad::current_direction() {
  uint8_t dir = 0x00;
  bool up, down, left, right;
  up = keys.at(KEY_UP).pressed_p;
  down = keys.at(KEY_DOWN).pressed_p;
  left = keys.at(KEY_LEFT).pressed_p;
  right = keys.at(KEY_RIGHT).pressed_p;

  if (up)   dir = (left) ? 0x07 : (right) ? 0x09 : 0x08;
  else if (down)  dir = (left) ? 0x01 : (right) ? 0x03 : 0x02;
  else if (right) dir = 0x06;
  else if (left)  dir = 0x04;

  return dir;
}

void Userpad::set_directions(DIDEVICEINSTANCE &di, void* di8_input, bool in_menu) {
  BYTE *keyboard = NULL;
  DIJOYSTATE2 *joystick = NULL;
  DWORD pov = 0;
  int type = GET_DIDEVICE_TYPE(di.dwDevType);

  if (type == DI8DEVTYPE_KEYBOARD) {
    keyboard = static_cast<BYTE *>(di8_input);
    keys.at(KEY_UP).pressed_p    = (keyboard[DIK_W] & 0x80) != 0;
    keys.at(KEY_DOWN).pressed_p  = (keyboard[DIK_S] & 0x80) != 0;
    keys.at(KEY_LEFT).pressed_p  = (keyboard[DIK_A] & 0x80) != 0;
    keys.at(KEY_RIGHT).pressed_p = (keyboard[DIK_D] & 0x80) != 0;
    keys.at(KEY_START).pressed_p = ((keyboard[DIK_RETURN] & 0x80) || (keyboard[DIK_NUMPADENTER] & 0x80)) != 0;

    if (idx >= 0 && direction_neutral_p() && in_menu) {
      for (int i = 0; i < 256; i++) {
        int was_mapped_p = keys.at(idx).mapped;
        if (keyboard[i] & 0x80) keys.at(idx).mapped = i;
        if (was_mapped_p == -1 && keys.at(idx).mapped != -1 && idx < 11 && keys.at(idx).pressed()) idx++;
      }
    }
  } else if (type == DI8DEVTYPE_JOYSTICK || type == DI8DEVTYPE_GAMEPAD || type == DIDEVTYPE_HID) {
    joystick = static_cast<DIJOYSTATE2 *>(di8_input);
    pov = joystick->rgdwPOV[0];

    const int DEAD = 100;
    // 789 = 31500, 0, 4500
    // 456 = 27000, -1, 9000
    // 123 = 22500, 18000, 13500
    keys.at(KEY_UP).pressed_p    = ((pov < 9000 - DEAD) || (pov > 27000 + DEAD && pov <= 36000)) || (joystick->lY < 0);
    keys.at(KEY_DOWN).pressed_p  = ((pov > 9000 + DEAD && pov < 27000 - DEAD)) || (joystick->lY > 0);
    keys.at(KEY_LEFT).pressed_p  = ((pov > 18000 + DEAD && pov < 36000 - DEAD)) || (joystick->lX < 0);
    keys.at(KEY_RIGHT).pressed_p = ((pov > DEAD && pov < 18000 - DEAD)) || (joystick->lX > 0);

    if (idx >= 0 && in_menu) {
      for (int i = 0; i < 128; i++) {
        int was_mapped_p = keys.at(idx).mapped;
        if (joystick->rgbButtons[i] & 0x80) keys.at(idx).mapped = i;
        else if (joystick->lZ < 0) keys.at(idx).mapped = 200;
        else if (joystick->lZ > 0) keys.at(idx).mapped = 201;

        if (was_mapped_p == -1 && keys.at(idx).mapped != -1 && idx < 11 && keys.at(idx).pressed()) idx++;
      }
    }
  }
}

bool joy_lz_p(DIJOYSTATE2 *joystick, uint8_t key) {
  if (key == 200 && joystick->lZ < 0) return true;
  if (key == 201 && joystick->lZ > 0) return true;

  return false;
}

void Userpad::set_buttons(DIDEVICEINSTANCE &di, void *di8_input, bool in_menu) {
  BYTE *keyboard = NULL;
  DIJOYSTATE2 *joystick = NULL;
  int type = GET_DIDEVICE_TYPE(di.dwDevType);

  if (type == DI8DEVTYPE_KEYBOARD) {
    keyboard = static_cast<BYTE *>(di8_input);
    for (int key = KEY_UP; key <= KEY_FN2; key++) {
      Input *input = &keys.at(key);

      input->pressed_p = (input->mapped != -1) ? (keyboard[input->mapped] & 0x80) : 0;
    }
  } else if (type == DI8DEVTYPE_JOYSTICK || type == DI8DEVTYPE_GAMEPAD || type == DIDEVTYPE_HID) {
    joystick = static_cast<DIJOYSTATE2 *>(di8_input);
    for (int key = KEY_UP; key <= KEY_FN2; key++) {
      Input *input = &keys.at(key);

      if (input->mapped != -1) {
        if (input->mapped > 128) input->pressed_p = joy_lz_p(joystick, input->mapped);
        else                     input->pressed_p = joystick->rgbButtons[input->mapped];
      }
    }
  }
}

void Userpad::set_addresses(PlayerSide player_side) {
  if (player_side == PlayerSide::LEFT) {
    keys.at(KEY_A).game_addr = (volatile uint8_t *)0x00771399;
    keys.at(KEY_A).menu_addr = (volatile uint8_t *)0x00771449;

    keys.at(KEY_B).game_addr = (volatile uint8_t *)0x0077139A;
    keys.at(KEY_B).menu_addr = (volatile uint8_t *)0x0077144A;

    keys.at(KEY_C).game_addr = (volatile uint8_t *)0x0077139B;
    keys.at(KEY_D).game_addr = (volatile uint8_t *)0x0077139C;
    keys.at(KEY_E).game_addr = (volatile uint8_t *)0x0077139D;

    keys.at(KEY_START).game_addr = (volatile uint8_t *)0x0077144B;

    keys.at(KEY_APLUSB).game_addr = (volatile uint8_t *)0x0077139E;
    keys.at(KEY_FN1).menu_addr    = (volatile uint8_t *)0x0077144C;
    keys.at(KEY_FN2).menu_addr    = (volatile uint8_t *)0x0077144D;
  } else if (player_side == PlayerSide::RIGHT) {
    keys.at(KEY_A).game_addr = (volatile uint8_t *)0x007713C5;
    keys.at(KEY_A).menu_addr = (volatile uint8_t *)0x00771475;

    keys.at(KEY_B).game_addr = (volatile uint8_t *)0x007713C6;
    keys.at(KEY_B).menu_addr = (volatile uint8_t *)0x00771476;

    keys.at(KEY_C).game_addr = (volatile uint8_t *)0x007713C7;
    keys.at(KEY_D).game_addr = (volatile uint8_t *)0x007713C8;
    keys.at(KEY_E).game_addr = (volatile uint8_t *)0x007713C9;

    keys.at(KEY_START).game_addr = (volatile uint8_t *)0x00771477;

    keys.at(KEY_APLUSB).game_addr = (volatile uint8_t *)0x00771C3A;
    keys.at(KEY_FN1).menu_addr    = (volatile uint8_t *)0x00771478;
    keys.at(KEY_FN2).menu_addr    = (volatile uint8_t *)0x00771479;
  } else {
    for (int key = KEY_UP; key <= KEY_FN2; key++) {
      keys.at(key).game_addr = NULL;
      keys.at(key).menu_addr = NULL;
    }
  }
}
