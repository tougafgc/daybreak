#ifndef RUNTIME_HPP__
#define RUNTIME_HPP__

#include "../lib/lib.hpp"

#define DIRECTINPUT_VERSION 0x0800
#include <winnt.h>
#include <dinput.h>

#include "squirrel.hpp"
#include "lisp.hpp"
#include "gamepad.hpp"

// DI8 vtable indicies
#define DI8_CREATEDEVICE 3
#define DI8_GETDEVICE_STATE 9
#define DI8_GETDEVICE_DATA 10

// D3DX9 vtable indices
#define DXD_RESET 16
#define DXD_PRESENT 17
#define DXD_ENDSCENE 42

typedef HRESULT(WINAPI* DI8Create_t)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
typedef HRESULT(WINAPI* DI8CreateDevice_t)(IDirectInput8*, REFGUID, IDirectInputDevice8**, LPUNKNOWN);
typedef HRESULT(WINAPI* DI8GetDeviceState_t)(IDirectInputDevice8*, DWORD, LPVOID);
typedef HRESULT(WINAPI *DI8GetDeviceData_t)(IDirectInputDevice8*, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);
typedef HRESULT(APIENTRY* Present_t)(LPDIRECT3DDEVICE9, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
typedef HRESULT(APIENTRY* Reset_t)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
typedef HRESULT(APIENTRY* EndScene_t)(LPDIRECT3DDEVICE9);

namespace runtime {
    extern bool g_controller_menu_p;
    extern bool g_in_menu_p;
    extern bool g_imgui_init_p;
    extern HWND g_mbaa_handle;
    extern LispController g_lisp;
    extern SquirrelController g_sq;
    extern GamepadController gamepad;
}

namespace hook {
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

  namespace di8 {
    extern DI8Create_t orig_create_fx;

    HRESULT WINAPI getDeviceState(IDirectInputDevice8*, DWORD, LPVOID);
    HRESULT WINAPI getDeviceData(IDirectInputDevice8*, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);
    HRESULT WINAPI createdevice(IDirectInput8*, REFGUID, IDirectInputDevice8**, LPUNKNOWN);
    HRESULT WINAPI create(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
  }

  namespace d3d {
    extern WNDPROC orig_wnd_proc;
    extern Present_t orig_present_fx;
    extern Reset_t orig_reset_fx;
    extern EndScene_t orig_endscene_fx;

    HRESULT APIENTRY present(LPDIRECT3DDEVICE9, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
    HRESULT APIENTRY reset(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
    HRESULT APIENTRY endscene(LPDIRECT3DDEVICE9);
    LRESULT CALLBACK wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param);
  }
}

#endif
