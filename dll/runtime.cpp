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
    Present_t orig_present_fx = nullptr;
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
        if (orig->u1.Ordinal & IMAGE_ORDINAL_FLAG) {
          continue;
        }

        auto *import = (IMAGE_IMPORT_BY_NAME *)((char *)module + orig->u1.AddressOfData);
        if (strcmp(import->Name, fx_name) != 0) {
          continue;
        }

        DWORD old;
        VirtualProtect(&thunk->u1.Function, sizeof(void*), PAGE_EXECUTE_READWRITE, &old);
        if (original) {
          *original = (void*)thunk->u1.Function;
        }

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
HRESULT WINAPI Hook::DI8::getDeviceState(IDirectInputDevice8* device, DWORD cbData, LPVOID lpData) {
  HRESULT hr = Hook::DI8::orig_getdevicestate_fx(device, cbData, lpData);
  if (FAILED(hr)) return hr;

  if (Runtime::in_menu_p && cbData == 256) {
    DIDEVICEINSTANCE di = {};
    di.dwSize = sizeof(di);
    if (Runtime::gamepad.show_menu() && SUCCEEDED(device->GetDeviceInfo(&di))) {
      for (auto& item : Runtime::gamepad.connected) {
        if (item.name == std::string(di.tszInstanceName)) {
          item.set_directions(di, lpData);
        }
      }
    }

    memset(lpData, 0, cbData);
  }

  return hr;
}

HRESULT WINAPI Hook::DI8::getDeviceData(IDirectInputDevice8* dev, DWORD size, LPDIDEVICEOBJECTDATA data, LPDWORD count, DWORD flags) {
  if (Runtime::in_menu_p && count) {
    *count = 0;
    return DI_OK;
  }

  return Hook::DI8::orig_getdevicedata_fx(dev, size, data, count, flags);
}

HRESULT WINAPI Hook::DI8::createdevice(IDirectInput8* self, REFGUID rguid, IDirectInputDevice8** device, LPUNKNOWN unk) {
  HRESULT hr = Hook::DI8::orig_createdevice_fx(self, rguid, device, unk);
  if (FAILED(hr)) return hr;

  if (rguid == GUID_SysKeyboard && device && *device &&
      !Hook::DI8::orig_getdevicestate_fx && !Hook::DI8::orig_getdevicedata_fx) {
    Hook::patch_fx<IDirectInputDevice8*, DI8GetDeviceState_t>(*device, DI8_GETDEVICE_STATE,
                                                              (void *)Hook::DI8::getDeviceState, Hook::DI8::orig_getdevicestate_fx);
    Hook::patch_fx<IDirectInputDevice8*, DI8GetDeviceData_t>(*device, DI8_GETDEVICE_DATA,
                                                             (void *)Hook::DI8::getDeviceData,  Hook::DI8::orig_getdevicedata_fx);
  }

  return hr;
}

HRESULT WINAPI Hook::DI8::create(HINSTANCE hinst, DWORD version, REFIID riid, LPVOID *out, LPUNKNOWN unk) {
  HRESULT hr = Hook::DI8::orig_create_fx(hinst, version, riid, out, unk);
  if (FAILED(hr)) return hr;

  IDirectInput8* di = (IDirectInput8*)(*out);

  if (!Hook::DI8::orig_createdevice_fx){
    Hook::patch_fx<IDirectInput8*, DI8CreateDevice_t>(di, DI8_CREATEDEVICE, (void*)Hook::DI8::createdevice, Hook::DI8::orig_createdevice_fx);
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

  // Consume all Win32 inputs so that Daybreak only has to process DI8.
  if (Runtime::in_menu_p) {
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui_ImplWin32_WndProcHandler(handle, msg, w_param, l_param))                      return TRUE;
    if (io.WantCaptureMouse && (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST))               return TRUE;
    if (io.WantCaptureKeyboard && (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR)) return TRUE;
  }

  return CallWindowProc(Hook::D3D::orig_wnd_proc, handle, msg, w_param, l_param);
}
