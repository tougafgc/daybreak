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

enum PlayerSide {
 LEFT = 0,
 MIDDLE = 1,
 RIGHT = 2
};

typedef struct Userpad {
  GUID id;
  std::string name;
  PlayerSide side;


} Userpad;

class GamepadController {
  public:
    GamepadController();
    ~GamepadController() = default;

    void init(HINSTANCE hinst);

    void draw_menu();

    void toggle_menu();

    void toggle_menu(bool state);

    bool show_menu();

    void add_controller(Userpad up);

  private:
    bool show_menu_p;
    IDirectInput8A *dinput;
    std::vector<Userpad> connected;
    std::optional<Userpad> leftSide;
    std::optional<Userpad> rightSide;
};
#endif
