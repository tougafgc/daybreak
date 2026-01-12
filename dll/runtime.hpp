#ifndef RUNTIME_HPP__
#define RUNTIME_HPP__

#include "../lib/lib.hpp"

#define DIRECTINPUT_VERSION 0x0800
#include <winnt.h>
#include <dinput.h>

#include "squirrel.hpp"
#include "lisp.hpp"
#include "gamepad.hpp"

// DI8
#define DI8_CREATEDEVICE 3
#define DI8_GETDEVICE_STATE 9
#define DI8_GETDEVICE_DATA 10

typedef HRESULT(WINAPI* DI8Create_t)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
typedef HRESULT(WINAPI* DI8CreateDevice_t)(LPDIRECTINPUT8, REFGUID, LPDIRECTINPUTDEVICE*, LPUNKNOWN);
typedef HRESULT(WINAPI* DI8GetDeviceState_t)(LPDIRECTINPUTDEVICE8, DWORD, LPVOID);
typedef HRESULT(WINAPI* DI8GetDeviceData_t)(LPDIRECTINPUTDEVICE8, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);

// D3D
#define D3D_PRESENT 17

typedef HRESULT(WINAPI* Present_t)(LPDIRECT3DDEVICE9, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);

//
namespace Runtime {
    extern bool controller_menu_p;
    extern bool in_menu_p;
    extern bool imgui_init_p;
    extern HWND mbaa_handle;
    extern LispController lisp;
    extern SquirrelController sq;
    extern GamepadController gamepad;
}

namespace Hook {
  //
  bool iat(HMODULE, const char*, const char*, void*, void**);

  //
  template <typename DEVICE_T, typename FUNC_T>
  void patch_fx(DEVICE_T device, int idx, void *callback, FUNC_T &orig) {
    if (!device) return;

    void** vtable = *(void***)(device);
    orig = (FUNC_T)vtable[idx];

    DWORD oldProtect;
    VirtualProtect(&vtable[idx], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
    vtable[idx] = callback;
    VirtualProtect(&vtable[idx], sizeof(void*), oldProtect, &oldProtect);
  }

  template <typename FX_T>
  void patch_table(void *table, int idx, void *callback, FX_T &orig) {
    void **vtable = *(void ***)(table);
    orig = (FX_T)vtable[idx];

    DWORD old;
    VirtualProtect(&vtable[idx], sizeof(void *), PAGE_EXECUTE_READWRITE, &old);
    vtable[idx] = callback;
    VirtualProtect(&vtable[idx], sizeof(void*), old, &old);
  }
}

namespace Hook::DI8 {
  extern DI8Create_t orig_create_fx;

  HRESULT WINAPI getDeviceState(LPDIRECTINPUTDEVICE8, DWORD, LPVOID);
  HRESULT WINAPI getDeviceData(LPDIRECTINPUTDEVICE8, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);
  HRESULT WINAPI createdevice(LPDIRECTINPUT8, REFGUID, LPDIRECTINPUTDEVICE8*, LPUNKNOWN);
  HRESULT WINAPI create(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
}
namespace Hook::D3D {
  extern WNDPROC orig_wnd_proc;
  extern Present_t orig_present_fx;

  HRESULT WINAPI present(LPDIRECT3DDEVICE9, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
  HRESULT WINAPI reset(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
  HRESULT WINAPI endscene(LPDIRECT3DDEVICE9);
  LRESULT CALLBACK wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param);
}

#endif
