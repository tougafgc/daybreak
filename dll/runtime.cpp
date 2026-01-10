#include "runtime.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param);

namespace runtime {
  bool g_controller_menu_p = false;
  bool g_in_menu_p = false;
  bool g_imgui_init_p = false;
  HWND g_mbaa_handle = nullptr;
  LispController g_lisp = LispController();
  SquirrelController g_sq = SquirrelController();
  GamepadController gamepad = GamepadController();
}

namespace hook {
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

  namespace melty {
    BattleScene_t orig_battlescene_fx = nullptr;

    void __stdcall draw_battlescene(int gs) {

      // GAME DOES NOT RUN PRESENT() DURING GAMEPLAY
      if (runtime::g_in_menu_p) {
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);

        if (runtime::gamepad.show_menu()) {
          ImGui::SetNextWindowSize(ImVec2(600, 385), ImGuiCond_FirstUseEver);
          runtime::gamepad.draw_menu();
        }
        else if (runtime::g_lisp.show_menu()) {
          ImGui::SetNextWindowSize(ImVec2(600, 385), ImGuiCond_FirstUseEver);
          runtime::g_lisp.draw_menu();
        }
        else if (runtime::g_sq.show_menu()) {
          ImGui::SetNextWindowSize(ImVec2(600, 385), ImGuiCond_FirstUseEver);
          runtime::g_sq.draw_menu();
        }

        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

        for (auto& pad : runtime::gamepad.connected) {
          pad.save_previous_keys();
        }
      }

      if (runtime::g_sq.script_active()) runtime::g_sq.call("draw");
      //if () g_lisp.evaluate("(draw)"); // TODO above

      orig_battlescene_fx(gs);
    }
  }

  namespace di8 {
    DI8Create_t orig_create_fx = nullptr;
    DI8CreateDevice_t orig_createdevice_fx = nullptr;
    DI8GetDeviceState_t orig_getdevicestate_fx = nullptr;
    DI8GetDeviceData_t orig_getdevicedata_fx = nullptr;

    HRESULT WINAPI getDeviceState(IDirectInputDevice8* device, DWORD cbData, LPVOID lpData) {
      HRESULT hr = hook::di8::orig_getdevicestate_fx(device, cbData, lpData);
      if (FAILED(hr)) return hr;

      if (runtime::g_in_menu_p && cbData == 256) {
        DIDEVICEINSTANCE di = {};
        di.dwSize = sizeof(di);
        if (runtime::gamepad.show_menu() && SUCCEEDED(device->GetDeviceInfo(&di))) {
          for (auto& item : runtime::gamepad.connected) {
            if (item.name == std::string(di.tszInstanceName)) {
              item.set_directions(di, lpData);
            }
          }
        }

        memset(lpData, 0, cbData);
      }

      return hr;
    }

    HRESULT WINAPI getDeviceData(IDirectInputDevice8* dev, DWORD size, LPDIDEVICEOBJECTDATA data, LPDWORD count, DWORD flags) {
      if (runtime::g_in_menu_p && count) {
        *count = 0;
        return DI_OK;
      }

      return hook::di8::orig_getdevicedata_fx(dev, size, data, count, flags);
    }

    HRESULT WINAPI createdevice(IDirectInput8* self, REFGUID rguid, IDirectInputDevice8** device, LPUNKNOWN unk) {
      HRESULT hr = hook::di8::orig_createdevice_fx(self, rguid, device, unk);
      if (FAILED(hr)) return hr;

      if (rguid == GUID_SysKeyboard && device && *device &&
          !hook::di8::orig_getdevicestate_fx && !hook::di8::orig_getdevicedata_fx) {
        hook::patch_fx<IDirectInputDevice8*, DI8GetDeviceState_t>(*device, DI8_GETDEVICE_STATE,
                                                                  (void *)hook::di8::getDeviceState, hook::di8::orig_getdevicestate_fx);
        hook::patch_fx<IDirectInputDevice8*, DI8GetDeviceData_t>(*device, DI8_GETDEVICE_DATA,
                                                                  (void *)hook::di8::getDeviceData,  hook::di8::orig_getdevicedata_fx);
      }

      return hr;
    }

    HRESULT WINAPI create(HINSTANCE hinst, DWORD version, REFIID riid, LPVOID *out, LPUNKNOWN unk) {
      HRESULT hr = hook::di8::orig_create_fx(hinst, version, riid, out, unk);
      if (FAILED(hr)) return hr;

      IDirectInput8* di = (IDirectInput8*)(*out);

      if (!hook::di8::orig_createdevice_fx){
        hook::patch_fx<IDirectInput8*, DI8CreateDevice_t>(di, DI8_CREATEDEVICE, (void*)hook::di8::createdevice, hook::di8::orig_createdevice_fx);
      }

      return hr;
    }
  }

  namespace d3d {
    Reset_t orig_reset_fx = nullptr;
    Present_t orig_present_fx = nullptr;
    EndScene_t orig_endscene_fx = nullptr;
    WNDPROC orig_wnd_proc = nullptr;

    HRESULT APIENTRY present(LPDIRECT3DDEVICE9 device, CONST RECT* src, CONST RECT* dest, HWND handle, CONST RGNDATA* region) {
      if (!hook::d3d::orig_present_fx) return D3D_OK;
      if (!device) return hook::d3d::orig_present_fx(device, src, dest, handle, region);
      if (device->TestCooperativeLevel() != D3D_OK) return hook::d3d::orig_present_fx(device, src, dest, handle, region);

      // GAME DOES NOT RUN PRESENT() DURING GAMEPLAY
      if (runtime::g_in_menu_p) {
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);

        if (runtime::gamepad.show_menu()) {
          ImGui::SetNextWindowSize(ImVec2(600, 385), ImGuiCond_FirstUseEver);
          runtime::gamepad.draw_menu();
        }
        else if (runtime::g_lisp.show_menu()) {
          ImGui::SetNextWindowSize(ImVec2(600, 385), ImGuiCond_FirstUseEver);
          runtime::g_lisp.draw_menu();
        }
        else if (runtime::g_sq.show_menu()) {
          ImGui::SetNextWindowSize(ImVec2(600, 385), ImGuiCond_FirstUseEver);
          runtime::g_sq.draw_menu();
        }

        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

        for (auto& pad : runtime::gamepad.connected) {
          pad.save_previous_keys();
        }
      }

      if (runtime::g_sq.script_active()) runtime::g_sq.call("draw");
      //if () g_lisp.evaluate("(draw)"); // TODO above

      return hook::d3d::orig_present_fx(device, src, dest, handle, region);
    }

    HRESULT APIENTRY endscene(LPDIRECT3DDEVICE9 device) {
      if (!hook::d3d::orig_endscene_fx) return D3D_OK;
      if (!device) return hook::d3d::orig_endscene_fx(device);

      return hook::d3d::orig_endscene_fx(device);
    }

    HRESULT reset(LPDIRECT3DDEVICE9 device, D3DPRESENT_PARAMETERS *pp) {
      if (!hook::d3d::orig_reset_fx) return D3D_OK;
      ImGui_ImplDX9_InvalidateDeviceObjects();
      HRESULT res = hook::d3d::orig_reset_fx(device, pp);

      if (SUCCEEDED(res)) ImGui_ImplDX9_CreateDeviceObjects();
      return res;
    }

    // D3DX9 Hook
    LRESULT CALLBACK wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param) {
      if (!hook::d3d::orig_wnd_proc) return DefWindowProc(handle, msg, w_param, l_param);
      if (runtime::g_in_menu_p) {
        ImGuiIO& io = ImGui::GetIO();
        if (ImGui_ImplWin32_WndProcHandler(handle, msg, w_param, l_param))                      return TRUE;
        if (io.WantCaptureMouse && (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST))               return TRUE;
        if (io.WantCaptureKeyboard && (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR)) return TRUE;
      }

      return CallWindowProc(hook::d3d::orig_wnd_proc, handle, msg, w_param, l_param);
    }
  }
}
