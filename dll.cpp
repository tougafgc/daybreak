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

// D3DX9 vtable indices
#define DXD_RESET 16
#define DXD_PRESENT 17
#define DXD_ENDSCENE 42

// Control macros
#define DEBOUNCE_PER 50

// Globals
LispController g_lisp = LispController();
SquirrelController g_sq = SquirrelController();

bool g_controller_menu_p = false;
bool g_in_menu_p = false;
bool g_imgui_init_p = false;

// D3DX9 Hook
typedef HRESULT(APIENTRY* Present_t)(LPDIRECT3DDEVICE9, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
typedef HRESULT(APIENTRY* Reset_t)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);

Reset_t orig_reset_fx = nullptr;
Present_t orig_present_fx = nullptr;
HWND g_mbaa_handle = nullptr;
WNDPROC g_orig_wnd_proc = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK hook_wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param) {
  if (g_in_menu_p) {
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui_ImplWin32_WndProcHandler(handle, msg, w_param, l_param))                      return TRUE;
    if (io.WantCaptureMouse && (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST))               return TRUE;
    if (io.WantCaptureKeyboard && (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR)) return TRUE;
  }

  return CallWindowProc(g_orig_wnd_proc, handle, msg, w_param, l_param);
}

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
      g_lisp.toggle_menu(IMGUI_HIDE);
      g_sq.toggle_menu(IMGUI_HIDE);
      g_controller_menu_p = !g_controller_menu_p;
      Sleep(DEBOUNCE_PER);
    }
    else if (keys_pressed(VK_CONTROL, 'K')) { // CTRL+K Lisp Menu
      g_lisp.toggle_menu();
      g_sq.toggle_menu(IMGUI_HIDE);
      g_controller_menu_p = false;
      Sleep(DEBOUNCE_PER);
    }
    else if (keys_pressed(VK_CONTROL, 'L')) { // CTRL+L Squirrel Menu
      g_lisp.toggle_menu(IMGUI_HIDE);
      g_sq.toggle_menu();
      g_controller_menu_p = false;
      Sleep(DEBOUNCE_PER);
    }
    else if (keys_pressed(VK_CONTROL, 'F')) { // CTRL+F, force close all menus
      g_lisp.toggle_menu(IMGUI_HIDE);
      g_sq.toggle_menu(IMGUI_HIDE);
      g_controller_menu_p = false;
    }

    g_in_menu_p = (g_controller_menu_p || g_lisp.show_menu() || g_sq.show_menu());

    Sleep(100);
  }
}

// ImGui hooking
HRESULT APIENTRY hook_present(LPDIRECT3DDEVICE9 device, CONST RECT* src, CONST RECT* dest, HWND handle, CONST RGNDATA* region) {
  if (!device || !g_imgui_init_p) return orig_present_fx(device, src, dest, handle, region);

  if (g_sq.script_active()) g_sq.call("draw");
  //if () g_lisp.evaluate("(draw)"); // TODO above

  if (g_in_menu_p) {
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);

    if (g_controller_menu_p) {
      ImGui::Begin("Controller", nullptr, ImGuiWindowFlags_NoTitleBar);
      ImGui::Text("Testing Controller Menu");
      ImGui::End();
    }
    else if (g_lisp.show_menu()) {
      ImGui::SetNextWindowSize(ImVec2(600, 385), ImGuiCond_FirstUseEver);
      g_lisp.draw_menu();
    }
    else if (g_sq.show_menu()) {
      ImGui::SetNextWindowSize(ImVec2(600, 385), ImGuiCond_FirstUseEver);
      g_sq.draw_menu();
    }

    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
  }

  return orig_present_fx(device, src, dest, handle, region);
}

HRESULT hook_reset(LPDIRECT3DDEVICE9 device, D3DPRESENT_PARAMETERS *pp) {
  ImGui_ImplDX9_InvalidateDeviceObjects();
  HRESULT res = orig_reset_fx(device, pp);

  if (SUCCEEDED(res)) ImGui_ImplDX9_CreateDeviceObjects();
  return res;
}


template <typename DXD_FUNC_T>
void hook_dxd_method(LPDIRECT3DDEVICE9 &device, int idx, void *callback, DXD_FUNC_T &orig) {
  if (!device) return;

  void** vtable = *(void***)(device);
  orig = (DXD_FUNC_T)vtable[idx];

  DWORD oldProtect;
  VirtualProtect(&vtable[idx], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
  vtable[idx] = callback;
  VirtualProtect(&vtable[idx], sizeof(void*), oldProtect, &oldProtect);
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

    g_orig_wnd_proc = (WNDPROC)SetWindowLongPtr(g_mbaa_handle, GWLP_WNDPROC, (LONG_PTR)hook_wnd_proc);

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui_ImplWin32_Init(g_mbaa_handle);
    ImGui_ImplDX9_Init(device);

    g_imgui_init_p = true;
  }

  hook_dxd_method<Present_t>(device, DXD_PRESENT, (void *)hook_present, orig_present_fx);
  hook_dxd_method<Reset_t>(device, DXD_RESET, (void *)hook_reset, orig_reset_fx);
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
  obj->init("daybreak\\autorun.crsc");
  obj->call("main");
  return 0;
}

DWORD WINAPI daybreak_threads(LPVOID param) {
  HMODULE hmodule = static_cast<HMODULE>(param);

  CreateThread(NULL, 0, server_wrapper, &g_lisp, 0, 0);
  CreateThread(NULL, 0, sq_wrapper, &g_sq, 0, 0);
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
      g_lisp.stop_server();
      break;
  }

  return TRUE;
}
