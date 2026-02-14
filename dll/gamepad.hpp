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
#define KEY_APLUSB 10
#define KEY_FN1 11
#define KEY_FN2 12

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

enum ControllerType {
  KEYBOARD = 0,
  JOYSTICK = 1
};

typedef struct Input {
  bool pressed_p;
  bool previous_p;
  int  mapped;
  volatile uint8_t *game_addr;
  volatile uint8_t *menu_addr;

  Input& operator=(const Input&) = default;

  void set(uint8_t val) {
    if (game_addr) {
      *game_addr = val;
    }

    if (menu_addr) {
      *menu_addr = (previous_p) ? 0x01 : 0x02;
    }
  }

  bool pressed() {
    return !previous_p && pressed_p;
  }
} Input;

class Userpad {
  public:
    Userpad(GUID id, const char *name, ControllerType type);
    ~Userpad() = default;

    GUID id;
    std::string name;
    PlayerSide side;
    State state;
    ControllerType type;
    int idx = -1;
    std::map<int, Input> keys;

    bool direction_neutral_p();
    uint8_t current_direction();

    void save_previous_keys();
    void set_directions(DIDEVICEINSTANCE &di, void *di8_input, bool in_menu);
    void set_buttons(DIDEVICEINSTANCE &di, void *di8_input, bool in_menu);
    void set_addresses(PlayerSide player_side);

    bool operator==(const Userpad &a) const {
      return name == a.name;
    }
};

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

    void update_connections();

    std::vector<Userpad> connected;

    bool left_free;
    bool right_free;

  private:
    bool show_menu_p;
    IDirectInput8A *dinput;
};
#endif
