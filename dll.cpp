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
      runtime::g_lisp.toggle_menu(IMGUI_HIDE);
      runtime::g_sq.toggle_menu(IMGUI_HIDE);
      runtime::gamepad.toggle_menu();
      Sleep(DEBOUNCE_PER);
    }
    else if (keys_pressed(VK_CONTROL, 'K')) { // CTRL+K Lisp Menu
      runtime::g_lisp.toggle_menu();
      runtime::g_sq.toggle_menu(IMGUI_HIDE);
      runtime::gamepad.toggle_menu(IMGUI_HIDE);
      Sleep(DEBOUNCE_PER);
    }
    else if (keys_pressed(VK_CONTROL, 'L')) { // CTRL+L Squirrel Menu
      runtime::g_lisp.toggle_menu(IMGUI_HIDE);
      runtime::g_sq.toggle_menu();
      runtime::gamepad.toggle_menu(IMGUI_HIDE);
      Sleep(DEBOUNCE_PER);
    }
    else if (keys_pressed(VK_CONTROL, 'F')) { // CTRL+F, force close all menus
      runtime::g_lisp.toggle_menu(IMGUI_HIDE);
      runtime::g_sq.toggle_menu(IMGUI_HIDE);
      runtime::gamepad.toggle_menu(IMGUI_HIDE);
    }

    runtime::g_in_menu_p = (runtime::gamepad.show_menu() || runtime::g_lisp.show_menu() || runtime::g_sq.show_menu());

    Sleep(100);
  }
}

DWORD WINAPI dxd_wrapper(LPVOID param) {
  HMODULE hmodule = static_cast<HMODULE>(param);

  while (*(LPDIRECT3DDEVICE9*)0x76E7D4 == nullptr) Sleep(100);
  LPDIRECT3DDEVICE9 device = *(LPDIRECT3DDEVICE9*)(0x76E7D4);

  if (!hook::d3d::orig_present_fx) {
    hook::patch_fx<LPDIRECT3DDEVICE9, Present_t>(device, DXD_PRESENT, (void *)hook::d3d::present, hook::d3d::orig_present_fx);
  }

  if (!hook::d3d::orig_endscene_fx) {
    hook::patch_fx<LPDIRECT3DDEVICE9, EndScene_t>(device, DXD_ENDSCENE, (void *)hook::d3d::endscene, hook::d3d::orig_endscene_fx);
  }

  if (!hook::d3d::orig_reset_fx) {
    hook::patch_fx<LPDIRECT3DDEVICE9, Reset_t>(device, DXD_RESET, (void *)hook::d3d::reset, hook::d3d::orig_reset_fx);
  }

  if (!runtime::g_imgui_init_p) {
    // Get window handle from swap chain
    IDirect3DSwapChain9* swap = nullptr;
    D3DPRESENT_PARAMETERS pp{};
    device->GetSwapChain(0, &swap);
    swap->GetPresentParameters(&pp);
    runtime::g_mbaa_handle = pp.hDeviceWindow;
    swap->Release();

    // Needed for ImGui to take in keypresses
    hook::d3d::orig_wnd_proc = (WNDPROC)SetWindowLongPtr(runtime::g_mbaa_handle, GWLP_WNDPROC, (LONG_PTR)hook::d3d::wnd_proc);

    runtime::gamepad.init(hmodule);

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui_ImplWin32_Init(runtime::g_mbaa_handle);
    ImGui_ImplDX9_Init(device);

    runtime::g_imgui_init_p = true;
  }

  return 0;
}

//DWORD WINAPI controller_wrapper(LPVOID) {}

DWORD WINAPI server_wrapper(LPVOID param) {
  runtime::g_lisp.init(L"daybreak\\newlisp.dll");
  runtime::g_lisp.load_file(std::string("daybreak/server.lsp"));
  runtime::g_lisp.start_server();
  return 0;
}

DWORD WINAPI sq_wrapper(LPVOID param) {
  runtime::g_sq.init("daybreak\\autorun.crsc");
  //runtime::g_sq.call("main");
  return 0;
}

DWORD WINAPI daybreak_threads(LPVOID param) {
  HMODULE hmodule = static_cast<HMODULE>(param);

  hook::iat(GetModuleHandle(nullptr), "dinput8.dll", "DirectInput8Create", (void *)hook::di8::create, (void**)&hook::di8::orig_create_fx);

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
      runtime::g_lisp.stop_server();
      break;
  }

  return TRUE;
}
