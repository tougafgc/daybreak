#ifndef GAMEPAD_HPP__
#define GAMEPAD_HPP__

#include "../lib/lib.hpp"
#include "../lib/imgui/imgui.h"
#include "../lib/imgui/imgui_impl_dx9.h"
#include "../lib/imgui/imgui_impl_win32.h"

#include <optional>

#define DIRECTINPUT_VERSION 0x0800
#include <winnt.h>
#include <dinput.h>

// There's gotta be an easier way
#define KEY_UP 0
#define KEY_DOWN 1
#define KEY_LEFT 2
#define KEY_RIGHT 3
#define KEY_A 4
#define KEY_B 5
#define KEY_C 6
#define KEY_D 7
#define KEY_E 8
#define KEY_START 9
#define KEY_FN1 10
#define KEY_FN2 11

enum PlayerSide {
 LEFT = 0,
 MIDDLE = 1,
 RIGHT = 2
};

enum State {
  NONE = 0,
  MAPPING = 1,
  TESTING = 2
};

typedef struct Userpad {
  GUID id;
  std::string name;
  PlayerSide side;
  State state;
  bool prev_keys[12] = { 0 };
  bool keys[12] = { 0 };

  int idx = -1;

  bool pressed(int key_id) {
    return !prev_keys[key_id] && keys[key_id];
  }

  void save_previous_keys() {
      prev_keys[KEY_UP]    = keys[KEY_UP];
      prev_keys[KEY_DOWN]  = keys[KEY_DOWN];
      prev_keys[KEY_LEFT]  = keys[KEY_LEFT];
      prev_keys[KEY_RIGHT] = keys[KEY_RIGHT];
      prev_keys[KEY_START] = keys[KEY_START];
  }

  void set_directions(DIDEVICEINSTANCE &di, void* di8_input) {
    BYTE *keyboard = NULL;
    DIJOYSTATE2 *joystick = NULL;
    DWORD pov = 0;

    switch (GET_DIDEVICE_TYPE(di.dwDevType)) {
    case DI8DEVTYPE_KEYBOARD:
      keyboard = static_cast<BYTE *>(di8_input);
      keys[KEY_UP]    = (keyboard[DIK_W] & 0x80) != 0;
      keys[KEY_DOWN]  = (keyboard[DIK_S] & 0x80) != 0;
      keys[KEY_LEFT]  = (keyboard[DIK_A] & 0x80) != 0;
      keys[KEY_RIGHT] = (keyboard[DIK_D] & 0x80) != 0;
      keys[KEY_START] = ((keyboard[DIK_RETURN] & 0x80) || (keyboard[DIK_NUMPADENTER] & 0x80)) != 0;
      break;
    case DI8DEVTYPE_JOYSTICK:
    case DI8DEVTYPE_GAMEPAD:
      // using DPAD
      joystick = static_cast<DIJOYSTATE2 *>(di8_input);
      pov = joystick->rgdwPOV[0];

      keys[KEY_UP]    = (pov == 0);
      keys[KEY_DOWN]  = (pov == 9000);
      keys[KEY_LEFT]  = (pov == 18000);
      keys[KEY_RIGHT] = (pov == 27000);
      break;
    default:
      break;
    }
  }

} Userpad;

class GamepadController {
  public:
    GamepadController();
    ~GamepadController() = default;

    void init(HINSTANCE hinst);

    void fsm(Userpad &side);

    void draw_side(std::optional<Userpad *> side, int offset);

    void draw_menu();

    void toggle_menu();

    void toggle_menu(bool state);

    bool show_menu();

    void add_controller(Userpad up);

    std::vector<Userpad> connected;

    bool left_free;
    bool right_free;

  private:
    bool show_menu_p;
    IDirectInput8A *dinput;
};
#endif
