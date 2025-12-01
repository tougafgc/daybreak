#include "lib.hpp"

#include "dll/imgui/imgui.h"
#include "dll/imgui/imgui_impl_dx9.h"
#include "dll/imgui/imgui_impl_win32.h"

#include "dll/server.hpp"
#include "dll/compiler.hpp"

// Globals
NewLispServer lisp = NewLispServer();
SquirrelCompiler sq = SquirrelCompiler();

bool controller_menu = false;
bool squirrel_menu = false;
bool lisp_menu = false;
bool in_menu = false;

// D3DX9 Hook
typedef HRESULT(APIENTRY* Present_t)(LPDIRECT3DDEVICE9, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
Present_t oPresent = nullptr;
bool g_ImGuiInitialized = false;
HWND g_Window = nullptr;

// Keyboard
template <typename... Key>
bool keys_pressed(Key... key) {
  return ((GetAsyncKeyState(key) & 0x8000) && ...);
}

DWORD WINAPI keyboard_wrapper(LPVOID hmodule) {
  while (true) {
    if (keys_pressed(VK_CONTROL, ';')) { // CTRL+;
      controller_menu = !controller_menu;
      lisp_menu = false;
      squirrel_menu = false;
      Sleep(200);
    }
    else if (keys_pressed(VK_CONTROL, 'L')) { // CTRL+;
      lisp_menu = !lisp_menu;
      controller_menu = false;
      squirrel_menu = false;
      Sleep(200);
    }
    else if (keys_pressed(VK_CONTROL, 'J')) { // CTRL+J
      squirrel_menu = !squirrel_menu;
      lisp_menu = false;
      controller_menu = false;
      Sleep(200);
    }
    else if (keys_pressed(VK_CONTROL, 'C')) { // CTRL+F
      controller_menu = false;
      lisp_menu = false;
      squirrel_menu = false;
    }

    in_menu = (controller_menu || lisp_menu || squirrel_menu);

    Sleep(100);
  }
}

// ImGui hooking
HRESULT APIENTRY hkPresent(LPDIRECT3DDEVICE9 device, CONST RECT* src, CONST RECT* dest, HWND hwnd, CONST RGNDATA* region) {
  if (!device || !g_ImGuiInitialized) return oPresent(device, src, dest, hwnd, region);

  sq.call("draw"); // TODO add guards if no file was loaded
  //lisp.evaluate("(draw)"); // TODO add autorun.lsp

  if (in_menu) {
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (controller_menu) {
      ImGui::Begin("Controller", nullptr, ImGuiWindowFlags_NoTitleBar);
      ImGui::Text("Testing Controller Menu");
      ImGui::End();
    }
    else if (lisp_menu) {
      lisp.draw_menu();
    }
    else if (squirrel_menu) {
      sq.draw_menu();
    }

    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
  }

  return oPresent(device, src, dest, hwnd, region);
}

void hook_present(LPDIRECT3DDEVICE9 &device) {
  if (!device) return;

  void** vtable = *(void***)(device);
  oPresent = (Present_t)vtable[17];

  DWORD oldProtect;
  VirtualProtect(&vtable[17], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
  vtable[17] = (void*)hkPresent;
  VirtualProtect(&vtable[17], sizeof(void*), oldProtect, &oldProtect);
}

DWORD WINAPI dxd_wrapper(LPVOID hmodule) {
  while (*(LPDIRECT3DDEVICE9*)0x76E7D4 == nullptr) Sleep(100);

  LPDIRECT3DDEVICE9 device = *(LPDIRECT3DDEVICE9*)(0x76E7D4);
  if (!g_ImGuiInitialized) {
    // Get window handle from swap chain
    IDirect3DSwapChain9* swap = nullptr;
    D3DPRESENT_PARAMETERS pp{};
    device->GetSwapChain(0, &swap);
    swap->GetPresentParameters(&pp);
    g_Window = pp.hDeviceWindow;
    swap->Release();

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(g_Window);
    ImGui_ImplDX9_Init(device);

    // ensure menus take up the full screen
    RECT rc{};
    GetClientRect(g_Window, &rc);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(rc.right - rc.left, rc.bottom - rc.top);
    ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);

    g_ImGuiInitialized = true;
  }

  hook_present(device);
  return 0;
}

DWORD WINAPI server_wrapper(LPVOID param) {
  NewLispServer *obj = static_cast<NewLispServer *>(param);
  obj->init(L"daybreak\\newlisp.dll");
  obj->load_file(std::string("daybreak/server.lsp"));
  return 0;
}

DWORD WINAPI sq_wrapper(LPVOID param) {
  SquirrelCompiler *obj = static_cast<SquirrelCompiler *>(param);
  obj->init("daybreak\\autorun.nut");
  obj->call("main");
  return 0;
}

DWORD WINAPI daybreak_threads(LPVOID param) {
  HMODULE hmodule = static_cast<HMODULE>(param);

  CreateThread(NULL, 0, server_wrapper, &lisp, 0, 0);
  CreateThread(NULL, 0, sq_wrapper, &sq, 0, 0);
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
