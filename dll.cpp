/**
 * dll.cpp
 * By: tougafgc
 * Date: 1 December 2025
 *
 * Top-level execution for when the DLL
 * is hooked into MBAA.exe. Handles Lisp
 * server setup, Squirrel execution, and
 * keyboard interactions.
 **/
#include "lib/lib.hpp"

#include "lib/imgui/imgui.h"
#include "lib/imgui/imgui_impl_dx9.h"
#include "lib/imgui/imgui_impl_win32.h"

#include "dll/runtime.hpp"

#define DEBOUNCE_PER 100

// Checks for either a single pressed key or a combination.
// This is done by folding, where N arguments fill N patterns
// of calling GetAsyncState(key), and then they are all compared
// together.
template <typename... Key>
bool keys_pressed(Key... key) {
  return ((GetAsyncKeyState(key) & 0x8000) && ...);
}

DWORD WINAPI keyboard_wrapper(LPVOID param) {
  while (true) {
    if (keys_pressed(VK_CONTROL, 'J')) { // CTRL+J Controller Menu
      Runtime::lisp.toggle_menu(IMGUI_HIDE);
      Runtime::sq.toggle_menu(IMGUI_HIDE);
      Runtime::gamepad.toggle_menu();
      Sleep(DEBOUNCE_PER);
    }
    else if (keys_pressed(VK_CONTROL, 'K')) { // CTRL+K Lisp Menu
      Runtime::lisp.toggle_menu();
      Runtime::sq.toggle_menu(IMGUI_HIDE);
      Runtime::gamepad.toggle_menu(IMGUI_HIDE);
      Sleep(DEBOUNCE_PER);
    }
    else if (keys_pressed(VK_CONTROL, 'L')) { // CTRL+L Squirrel Menu
      Runtime::lisp.toggle_menu(IMGUI_HIDE);
      Runtime::sq.toggle_menu();
      Runtime::gamepad.toggle_menu(IMGUI_HIDE);
      Sleep(DEBOUNCE_PER);
    }
    else if (keys_pressed(VK_CONTROL, 'F')) { // CTRL+F, force close all menus
      Runtime::lisp.toggle_menu(IMGUI_HIDE);
      Runtime::sq.toggle_menu(IMGUI_HIDE);
      Runtime::gamepad.toggle_menu(IMGUI_HIDE);
    }

    Runtime::in_menu_p = (Runtime::gamepad.show_menu() || Runtime::lisp.show_menu() || Runtime::sq.show_menu());

    Sleep(100);
  }
}

  void tramp_single(uintptr_t epilogue_addr, void *callback) {
    DWORD old;
    VirtualProtect((void*)epilogue_addr, 6, PAGE_EXECUTE_READWRITE, &old);

    // JMP callback
    *(uint8_t*)epilogue_addr = 0xE9;
    *(uintptr_t*)(epilogue_addr + 1) = (uintptr_t)callback - (epilogue_addr + 5);

    // NOP leftover byte (original instruction was 6 bytes)
    *(uint8_t*)(epilogue_addr + 5) = 0x90;
    VirtualProtect((void*)epilogue_addr, 6, old, &old);
  }

extern "C" void test() {
  for (auto& item : Runtime::gamepad.connected) {
    if (Runtime::in_menu_p || Runtime::gamepad.left_free) {
      *(volatile char *)0x00771398 = 0x00;
      *(volatile char *)0x00771448 = 0x00;
      *(volatile char *)0x0077146A &= 0xF0;
      *(volatile char *)0x0077151A &= 0xF0;
    }

    if (Runtime::in_menu_p || Runtime::gamepad.right_free) {
      *(volatile char *)0x007713C4 = 0x00;
      *(volatile char *)0x00771496 &= 0xF0;
    }

    // only needs to set, reset is handled by the game
    if (item.side != PlayerSide::MIDDLE && !Runtime::in_menu_p) {
      char dir = item.current_direction();

      if (item.side == PlayerSide::LEFT) {
        *(volatile char *)0x00771398 = dir;
//        *(volatile char *)0x00771448 = dir;

        // Stage wants 0xf, so does main menu
        *(volatile char *)0x0077146A = (*(volatile char *)0x0077146A & 0xF0) + dir;
        *(volatile char *)0x0077151A = (*(volatile char *)0x0077146A & 0xF0) + dir;

        for (int key = KEY_UP; key <= KEY_FN2; key++) {
          Input *input = &item.keys.at(key);

          if (input->pressed_p) input->set(0x01);
        }
      } else if (item.side == PlayerSide::RIGHT) {
        *(volatile char *)0x007713C4 = dir;

        // Stage wants 0xf, so does main menu
        *(volatile char *)0x00771496 = (*(volatile char *)0x00771496 & 0xF0) + dir;

        for (int key = KEY_UP; key <= KEY_FN2; key++) {
          Input *input = &item.keys.at(key);

          if (input->pressed_p) input->set(0x01);
        }
      }
    }
  }
}

__attribute__((naked)) void readcontrollerinputs() {
  __asm__ __volatile__ (
    "pushal\n\t"
    "pushfl\n\t"

    "call _test\n\t"

    "popfl\n\t"
    "popal\n\t"

    "add $0x90, %esp\n\t"
    "ret\n\t"
  );
}

DWORD WINAPI dxd_wrapper(LPVOID param) {
  HMODULE hmodule = static_cast<HMODULE>(param);

  while (*(LPDIRECT3DDEVICE9*)0x76E7D4 == nullptr) Sleep(100);
  LPDIRECT3DDEVICE9 device = *(LPDIRECT3DDEVICE9*)(0x0076E7D4);

  Hook::trampoline<LPDIRECT3DDEVICE9, D3DPresent_t>(device, 17, (void *)Hook::D3D::present, Hook::D3D::orig_present_fx);

  tramp_single(0x0041F1A6, (void *)readcontrollerinputs);

  if (!Runtime::imgui_init_p) {
    // Get window handle from swap chain
    IDirect3DSwapChain9* swap = nullptr;
    D3DPRESENT_PARAMETERS pp{};
    device->GetSwapChain(0, &swap);
    swap->GetPresentParameters(&pp);
    Runtime::mbaa_handle = pp.hDeviceWindow;
    swap->Release();

    // Needed for ImGui to take in keypresses
    Hook::D3D::orig_wnd_proc = (WNDPROC)SetWindowLongPtr(Runtime::mbaa_handle, GWLP_WNDPROC, (LONG_PTR)Hook::D3D::wnd_proc);

    Runtime::gamepad.init(hmodule);

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui_ImplWin32_Init(Runtime::mbaa_handle);
    ImGui_ImplDX9_Init(device);

    Runtime::imgui_init_p = true;
  }

  return 0;
}

//DWORD WINAPI controller_wrapper(LPVOID) {}

DWORD WINAPI server_wrapper(LPVOID param) {
  Runtime::lisp.init(L"daybreak\\newlisp.dll");
  Runtime::lisp.load_file(std::string("daybreak/server.lsp"));
  Runtime::lisp.start_server();
  return 0;
}

DWORD WINAPI sq_wrapper(LPVOID param) {
  Runtime::sq.init("daybreak\\autorun.crsc");
  //Runtime::sq.call("main");
  return 0;
}

DWORD WINAPI daybreak_threads(LPVOID param) {
  HMODULE hmodule = static_cast<HMODULE>(param);

  Hook::iat(GetModuleHandle(nullptr), "dinput8.dll", "DirectInput8Create", (void *)Hook::DI8::create, (void**)&Hook::DI8::orig_create_fx);

  CreateThread(NULL, 0, server_wrapper, hmodule, 0, 0);
  CreateThread(NULL, 0, sq_wrapper, hmodule, 0, 0);
  CreateThread(NULL, 0, keyboard_wrapper, hmodule, 0, 0);
  CreateThread(NULL, 0, dxd_wrapper, hmodule, 0, 0);

  return 0;
}

BOOL WINAPI DllMain(HMODULE hmodule, DWORD reason, LPVOID reserved) {
  switch (reason) {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(hmodule);
      CreateThread(NULL, 0, daybreak_threads, hmodule, 0, 0);
      break;

    case DLL_PROCESS_DETACH:
      Runtime::lisp.stop_server();
      break;
  }

  return TRUE;
}
