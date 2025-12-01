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

#include "dll/lisp.hpp"
#include "dll/squirrel.hpp"

// Globals
LispController lisp = LispController();
SquirrelController sq = SquirrelController();

bool controller_menu = false;
bool in_menu = false;

// D3DX9 Hook
typedef HRESULT(APIENTRY* Present_t)(LPDIRECT3DDEVICE9, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
Present_t orig_present_fx = nullptr;
bool g_imgui_init_p = false;
HWND g_mbaa_handle = nullptr;

// Keyboard

// Checks for either a single pressed key or a combination.
// This is done by folding, where N arguments fill N patterns
// of calling GetAsyncState(key), and then they are all compared
// together.
template <typename... Key>
bool keys_pressed(Key... key) {
  return ((GetAsyncKeyState(key) & 0x8000) && ...);
}

DWORD WINAPI keyboard_wrapper(LPVOID hmodule) {
  while (true) {
    if (keys_pressed(VK_CONTROL, 'J')) { // CTRL+J Controller Menu
      lisp.toggle_menu(IMGUI_HIDE);
      sq.toggle_menu(IMGUI_HIDE);
      controller_menu = !controller_menu;
      Sleep(200);
    }
    else if (keys_pressed(VK_CONTROL, 'L')) { // CTRL+L Lisp Menu
      lisp.toggle_menu();
      sq.toggle_menu(IMGUI_HIDE);
      controller_menu = false;
      Sleep(200);
    }
    else if (keys_pressed(VK_CONTROL, ';')) { // CTRL+; Squirrel Menu
      lisp.toggle_menu(IMGUI_HIDE);
      sq.toggle_menu();
      controller_menu = false;
      Sleep(200);
    }
    else if (keys_pressed(VK_CONTROL, 'F')) { // CTRL+F, force close all menus
      lisp.toggle_menu(IMGUI_HIDE);
      sq.toggle_menu(IMGUI_HIDE);
      controller_menu = false;
    }

    in_menu = (controller_menu || lisp.show_menu() || sq.show_menu());

    Sleep(100);
  }
}

// ImGui hooking
HRESULT APIENTRY hkPresent(LPDIRECT3DDEVICE9 device, CONST RECT* src, CONST RECT* dest, HWND hwnd, CONST RGNDATA* region) {
  if (!device || !g_imgui_init_p) return orig_present_fx(device, src, dest, hwnd, region);

  sq.call("draw"); // TODO add guards if no file was loaded
  //lisp.evaluate("(draw)"); // TODO above

  if (in_menu) {
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (controller_menu) {
      ImGui::Begin("Controller", nullptr, ImGuiWindowFlags_NoTitleBar);
      ImGui::Text("Testing Controller Menu");
      ImGui::End();
    }
    else if (lisp.show_menu()) {
      lisp.draw_menu();
    }
    else if (sq.show_menu()) {
      sq.draw_menu();
    }

    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
  }

  return orig_present_fx(device, src, dest, hwnd, region);
}

void hook_present(LPDIRECT3DDEVICE9 &device) {
  if (!device) return;

  void** vtable = *(void***)(device);
  orig_present_fx = (Present_t)vtable[17];

  DWORD oldProtect;
  VirtualProtect(&vtable[17], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
  vtable[17] = (void*)hkPresent;
  VirtualProtect(&vtable[17], sizeof(void*), oldProtect, &oldProtect);
}

DWORD WINAPI dxd_wrapper(LPVOID hmodule) {
  while (*(LPDIRECT3DDEVICE9*)0x76E7D4 == nullptr) Sleep(100);

  LPDIRECT3DDEVICE9 device = *(LPDIRECT3DDEVICE9*)(0x76E7D4);
  if (!g_imgui_init_p) {
    // Get window handle from swap chain
    IDirect3DSwapChain9* swap = nullptr;
    D3DPRESENT_PARAMETERS pp{};
    device->GetSwapChain(0, &swap);
    swap->GetPresentParameters(&pp);
    g_mbaa_handle = pp.hDeviceWindow;
    swap->Release();

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(g_mbaa_handle);
    ImGui_ImplDX9_Init(device);

    // ensure menus take up the full screen
    RECT rc{};
    GetClientRect(g_mbaa_handle, &rc);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(rc.right - rc.left, rc.bottom - rc.top);
    ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);

    g_imgui_init_p = true;
  }

  hook_present(device);
  return 0;
}

DWORD WINAPI server_wrapper(LPVOID param) {
  LispController *obj = static_cast<LispController *>(param);
  obj->init(L"daybreak\\newlisp.dll");
  obj->load_file(std::string("daybreak/server.lsp"));
  obj->start_server();
  return 0;
}

DWORD WINAPI sq_wrapper(LPVOID param) {
  SquirrelController *obj = static_cast<SquirrelController *>(param);
  obj->init("daybreak\\autorun.nut");
  obj->call("main");
  return 0;
}

DWORD WINAPI daybreak_threads(LPVOID param) {
  HMODULE hmodule = static_cast<HMODULE>(param);

  CreateThread(NULL, 0, server_wrapper, &lisp, 0, 0);
  CreateThread(NULL, 0, sq_wrapper, &sq, 0, 0);
  //CreateThread(NULL, 0, lisp_wrapper, &sq, 0, 0); // Used for if/when autorun.lsp exists
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
      lisp.stop_server();
      break;
  }

  return TRUE;
}
