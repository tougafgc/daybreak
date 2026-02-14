#include "runtime.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param);

namespace Runtime {
  bool controller_menu_p = false;
  bool in_menu_p = false;
  bool imgui_init_p = false;
  HWND mbaa_handle = nullptr;
  LispController lisp = LispController();
  SquirrelController sq = SquirrelController();
  GamepadController gamepad = GamepadController();
}

namespace Hook {
  namespace DI8 {
    DI8Create_t orig_create_fx = nullptr;
    DI8CreateDevice_t orig_createdevice_fx = nullptr;
    DI8GetDeviceState_t orig_getdevicestate_fx = nullptr;
    DI8GetDeviceData_t orig_getdevicedata_fx = nullptr;
  }

  namespace D3D {
    D3DPresent_t orig_present_fx = nullptr;
    WNDPROC orig_wnd_proc = nullptr;
  }

  bool iat(HMODULE module, const char *dll_name, const char *fx_name, void *hook, void **original) {
    auto *dos = (IMAGE_DOS_HEADER *)module;
    auto *nt  = (IMAGE_NT_HEADERS *)((char *)module + dos->e_lfanew);
    auto &dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    auto *imp = (IMAGE_IMPORT_DESCRIPTOR *)((char *)module + dir.VirtualAddress);

    for (; imp->Name; imp++) {
      const char *name = (char *)module + imp->Name;
      if (_stricmp(name, dll_name) != 0) {
        continue;
      }

      auto *thunk = (IMAGE_THUNK_DATA *)((char *)module + imp->FirstThunk);
      auto *orig  = (IMAGE_THUNK_DATA *)((char *)module + imp->OriginalFirstThunk);

      for (; orig->u1.AddressOfData; orig++, thunk++) {
        DWORD old;
        if (orig->u1.Ordinal & IMAGE_ORDINAL_FLAG) continue;

        auto *import = (IMAGE_IMPORT_BY_NAME *)((char *)module + orig->u1.AddressOfData);
        if (strcmp(import->Name, fx_name) != 0) continue;

        VirtualProtect(&thunk->u1.Function, sizeof(void*), PAGE_EXECUTE_READWRITE, &old);
        if (original) *original = (void*)thunk->u1.Function;

        thunk->u1.Function = (ULONG_PTR)hook;
        VirtualProtect(&thunk->u1.Function, sizeof(void*), old, &old);

        return true;
      }
    }

    return false;
  }
}


/**
 * Direct Input Hooks
 */
HRESULT WINAPI Hook::DI8::getDeviceState(LPDIRECTINPUTDEVICE8 device, DWORD cbData, LPVOID lpData) {
  HRESULT hr = Hook::DI8::orig_getdevicestate_fx(device, cbData, lpData);
  if (FAILED(hr)) return hr;
  if (cbData == 256 || cbData == 272) {
    DIDEVICEINSTANCE di = {};
    di.dwSize = sizeof(di);

    if (SUCCEEDED(device->GetDeviceInfo(&di))) {
      for (auto& item : Runtime::gamepad.connected) {
        if (item.name == std::string(di.tszInstanceName)) {
          item.set_buttons(di, lpData, Runtime::in_menu_p);
          item.set_directions(di, lpData, Runtime::in_menu_p);
           // Disable all inputs that aren't directly caught/wanted/etc by Daybreak
          memset(lpData, 0, cbData);
            
	  /* /if (item.side == PlayerSide::LEFT && !Runtime::in_menu_p) {
            // set MBAACC player structs
            // P1
            //   Numpad Dir = 398 & 448
            //   A       = 00771399
            //   B       = 39A
            //   C       = 39B
            //   D       = 39C
            //   E       = 39D
            //   A+B     = 39E
            //   Confirm = 449
            //   Cancel  = 44A
            //   Start   = 44B
            //   FN1     = 44C
            //   FN2     = 44D
            //if (item.keys[KEY_A]) *(volatile char *)0x00771399 = (char)0x02;
            *(volatile uint8_t *)0x00771399 = (item.keys[KEY_A]) ? 3 : 0x00;
            *(volatile uint8_t *)0x0077139A = (item.keys[KEY_B]) ? 0x01 : 0x00;
            *(volatile uint8_t *)0x0077139B = (item.keys[KEY_C]) ? 0x01 : 0x00;
            *(volatile uint8_t *)0x0077139C = (item.keys[KEY_D]) ? 0x01 : 0x00;
            *(volatile uint8_t *)0x0077139D = (item.keys[KEY_E]) ? 0x01 : 0x00;
            //0x0077139E = item.keys[KEY_APLUSB]);
            *(volatile char *)0x00771449 = (item.keys[KEY_A]) ? 0x03 : 0x00;
            *(volatile uint8_t *)0x0077144A = (item.keys[KEY_B]) ? 0x01 : 0x00;
            *(volatile uint8_t *)0x0077144B = (item.keys[KEY_START]) ? 0x01 : 0x00;
            *(volatile uint8_t *)0x0077144C = (item.keys[KEY_FN1]) ? 0x01 : 0x00;
            *(volatile uint8_t *)0x0077144D = (item.keys[KEY_FN2]) ? 0x01 : 0x00;
          } else if (item.side == PlayerSide::RIGHT && !Runtime::in_menu_p) {
            //
            // P2
            // Dir       = 3C4
            //   A       = 007713C5
            //   B       = 3C6
            //   C       = 3C7
            //   D       = 3C8
            //   E       = 3C9
            //   A+B     = 3CA
            //   Confirm = 525
            //   Cancel  = 526
            //   Start   = 477
            //   FN1     = 478
            //   FN2     = 479
            *(volatile uint8_t *)0x007713C5 = (item.keys[KEY_A]) ? 0x01 : 0x00;
            *(volatile uint8_t *)0x007713C6 = (item.keys[KEY_B]) ? 0x01 : 0x00;
            *(volatile uint8_t *)0x007713C7 = (item.keys[KEY_C]) ? 0x01 : 0x00;
            *(volatile uint8_t *)0x007713C8 = (item.keys[KEY_D]) ? 0x01 : 0x00;
            *(volatile uint8_t *)0x007713C9 = (item.keys[KEY_E]) ? 0x01 : 0x00;
            //0x0077139E = item.keys[KEY_APLUSB]);
            *(volatile uint8_t *)0x00771525 = (item.keys[KEY_A]) ? 0x01 : 0x00;
            *(volatile uint8_t *)0x00771526 = (item.keys[KEY_B]) ? 0x01 : 0x00;
            *(volatile uint8_t *)0x00771477 = (item.keys[KEY_START]) ? 0x01 : 0x00;
            *(volatile uint8_t *)0x00771448 = (item.keys[KEY_FN1]) ? 0x01 : 0x00;
            *(volatile uint8_t *)0x00771449 = (item.keys[KEY_FN2]) ? 0x01 : 0x00;
          }*/
        }
      }
    }
  }

  return hr;
}

HRESULT WINAPI Hook::DI8::getDeviceData(LPDIRECTINPUTDEVICE8 dev, DWORD size, LPDIDEVICEOBJECTDATA data, LPDWORD count, DWORD flags) {
  if (Runtime::in_menu_p && count) {
    *count = 0;
    return DI_OK;
  }

  return Hook::DI8::orig_getdevicedata_fx(dev, size, data, count, flags);
}

HRESULT WINAPI Hook::DI8::createDevice(LPDIRECTINPUT8 self, REFGUID rguid, LPDIRECTINPUTDEVICE8* device, LPUNKNOWN unk) {
  HRESULT hr = Hook::DI8::orig_createdevice_fx(self, rguid, device, unk);
  if (FAILED(hr)) return hr;

  if (rguid == GUID_SysKeyboard && device && *device && !Hook::DI8::orig_getdevicestate_fx && !Hook::DI8::orig_getdevicedata_fx) {
    Hook::patch_fx<LPDIRECTINPUTDEVICE8, DI8GetDeviceState_t>(*device, DI8_GETDEVICE_STATE, (void *)Hook::DI8::getDeviceState, Hook::DI8::orig_getdevicestate_fx);
    Hook::patch_fx<LPDIRECTINPUTDEVICE8, DI8GetDeviceData_t>(*device, DI8_GETDEVICE_DATA, (void *)Hook::DI8::getDeviceData,  Hook::DI8::orig_getdevicedata_fx);
  }

  return hr;
}

HRESULT WINAPI Hook::DI8::create(HINSTANCE hinst, DWORD version, REFIID riid, LPVOID *out, LPUNKNOWN unk) {
  HRESULT hr = Hook::DI8::orig_create_fx(hinst, version, riid, out, unk);
  if (FAILED(hr)) return hr;

  LPDIRECTINPUT8 di = (LPDIRECTINPUT8)(*out);

  if (!Hook::DI8::orig_createdevice_fx){
    Hook::patch_fx<LPDIRECTINPUT8, DI8CreateDevice_t>(di, DI8_CREATEDEVICE, (void*)Hook::DI8::createDevice, Hook::DI8::orig_createdevice_fx);
  }

  return hr;
}

/**
 * Direct 3D Hooks
 */
HRESULT WINAPI Hook::D3D::present(LPDIRECT3DDEVICE9 device, CONST RECT* src, CONST RECT* dest, HWND handle, CONST RGNDATA* region) {
  if (!Hook::D3D::orig_present_fx) return D3D_OK;
  if (!device) return Hook::D3D::orig_present_fx(device, src, dest, handle, region);
  if (device->TestCooperativeLevel() != D3D_OK) return Hook::D3D::orig_present_fx(device, src, dest, handle, region);

  /**
   * Per-frame ImGui menu logic
   */
  if (Runtime::in_menu_p) {
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 385), ImGuiCond_FirstUseEver);

    if (Runtime::gamepad.show_menu())     Runtime::gamepad.draw_menu();
    else if (Runtime::lisp.show_menu()) Runtime::lisp.draw_menu();
    else if (Runtime::sq.show_menu())   Runtime::sq.draw_menu();

    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
  }

  /**
   * Per-frame input logic
   */
  for (auto& pad : Runtime::gamepad.connected) {
    pad.save_previous_keys();
  }

  /**
   * Per-frame scripting logic
   */
  if (Runtime::sq.script_active()) Runtime::sq.call("draw");
  //if () lisp.evaluate("(draw)"); // TODO above

  return Hook::D3D::orig_present_fx(device, src, dest, handle, region);
}

LRESULT CALLBACK Hook::D3D::wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param) {
  if (!Hook::D3D::orig_wnd_proc) return DefWindowProc(handle, msg, w_param, l_param);

  if (Runtime::in_menu_p) {
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui_ImplWin32_WndProcHandler(handle, msg, w_param, l_param))                      return TRUE;
    if (io.WantCaptureMouse && (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST))               return TRUE;
    if (io.WantCaptureKeyboard && (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR)) return TRUE;
  }

  return CallWindowProc(Hook::D3D::orig_wnd_proc, handle, msg, w_param, l_param);
}
