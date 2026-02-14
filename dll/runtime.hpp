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
typedef HRESULT(WINAPI* DI8CreateDevice_t)(LPDIRECTINPUT8, REFGUID, LPDIRECTINPUTDEVICE8*, LPUNKNOWN);
typedef HRESULT(WINAPI* DI8GetDeviceState_t)(LPDIRECTINPUTDEVICE8, DWORD, LPVOID);
typedef HRESULT(WINAPI* DI8GetDeviceData_t)(LPDIRECTINPUTDEVICE8, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);

// D3D
#define D3D_PRESENT 17

typedef HRESULT(WINAPI* D3DPresent_t)(LPDIRECT3DDEVICE9, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);

/**
 * Runtime namespace for managing global variables, states and menuing.
 *
 * controller_menu_p - Flag reflecting if the Daybreak Controller Configuration menu is currently present on screen.
 * in_menu_p         - Flag reflecting if a Daybreak menu is currently present on screen.
 * imgui_init_p      - Flag reflecting the initalization status of the ImGui instance. Daybreak will not display menus if this is false.
 * mbaa_handle       - Handle to the active Melty Blood window.
 * lisp              - Controller for the Lisp scripting aspect of Daybreak.
 * sq                - Controller for the Squirrel scripting aspect of Daybreak.
 * gamepad           - Controller for the Controller Configuration menu & functionality.
 */
namespace Runtime {
    extern bool controller_menu_p;
    extern bool in_menu_p;
    extern bool imgui_init_p;
    extern HWND mbaa_handle;
    extern LispController lisp;
    extern SquirrelController sq;
    extern GamepadController gamepad;
}

/**
 * Hook namespace for hooking functions in DirectInput and Direct3D. Uses IAT hooking
 * for functions that can't be patched in memory (DirectInput), and inline trampolining
 * everywhere else.
 */
namespace Hook {
  /**
   * Import Address Table hook. Saves the original function and replaces it with the custom hooked
   * function.
   *
   * https://trikkss.github.io/posts/iat_hooking/
   *
   * @param module    Module Handle of the current process/window (Melty Blood).
   * @param dll_name  Name of the DLL to replace a function.
   * @param fx_name   Function name inside the DLL to replace.
   * @param hook      Function to be injected.
   * @param original  Save the original function so that it can be used inside the custom hook.
   * @return true if the hook was successful, false otherwise.
   */
  bool iat(HMODULE module, const char *dll_name, const char *fx_name, void *hook, void **original);

  /**
   * Patch a function using the inline trampoline method. If a vtable entry does not
   * exist, providing an address can also work (TODO, implement trampolining for bare addresses).
   *
   * https://blog.securehat.co.uk/process-injection/manually-implementing-inline-function-hooking
   * https://unprotect.it/technique/inline-hooking/
   *
   * @tparam CLASS_T Type of variable/class object pointer.
   * @tparam FUNC_T  Type signature of the member function.
   * @param device   Variable containing the class object to get its vtable from.
   * @param idx      InDeX in the vtable that stores the desired function to hook.
   * @param callback Custom function to replace the vtable entry.
   * @param orig     Save the original function so that it can be used inside of the custom function.
   */
  template <typename CLASS_T, typename FUNC_T>
  void trampoline(CLASS_T device, int idx, void *callback, FUNC_T &orig) {
    // 5 is return offset, TODO make this not a magic number.
    DWORD old;
    DWORD fx = (*(DWORD **)device)[idx];
    uintptr_t return_addr = fx + 5;

    auto *trampoline = (uint8_t *)VirtualAlloc(nullptr, 5 + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    memcpy(trampoline, (void *)fx, 5);

    trampoline[5] = 0xE9;
    *(uintptr_t *)(trampoline + 5 + 1) = return_addr - (uintptr_t)(trampoline + 5) - 5;

    orig = (FUNC_T)trampoline;

    VirtualProtect((void *)fx, 5, PAGE_EXECUTE_READWRITE, &old);
    *(uint8_t *)fx = 0xE9;
    *(uintptr_t *)(fx + 1) = (uintptr_t)callback - fx - 5;
    VirtualProtect((void *)fx, 5, old, &old);
  }

  /**
   * TODO make this accurate
   *
   * Patch a function using the inline trampoline method. If a vtable entry does not
   * exist, providing an address can also work (TODO, implement trampolining for bare addresses).
   *
   * https://blog.securehat.co.uk/process-injection/manually-implementing-inline-function-hooking
   * https://unprotect.it/technique/inline-hooking/
   *
   * @tparam CLASS_T Type of variable/class object pointer.
   * @tparam FUNC_T  Type signature of the member function.
   * @param device   Variable containing the class object to get its vtable from.
   * @param idx      InDeX in the vtable that stores the desired function to hook.
   * @param callback Custom function to replace the vtable entry.
   * @param orig     Save the original function so that it can be used inside of the custom function.
   */
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
}

/**
 * Hook namespace section for DirectInput.
 */
namespace Hook::DI8 {
  extern DI8Create_t orig_create_fx; // function pointer storage for DI8::Create.

  /**
   *
   */
  HRESULT WINAPI getDeviceState(LPDIRECTINPUTDEVICE8, DWORD, LPVOID);

  /**
   *
   */
  HRESULT WINAPI getDeviceData(LPDIRECTINPUTDEVICE8, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);

  /**
   *
   */
  HRESULT WINAPI createDevice(LPDIRECTINPUT8, REFGUID, LPDIRECTINPUTDEVICE8*, LPUNKNOWN);

  /**
   *
   */
  HRESULT WINAPI create(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
}

/**
 * Hook namespace section for Direct3D.
 */
namespace Hook::D3D {
  extern WNDPROC orig_wnd_proc;         // function pointer storage for Win32 WNDPROC callback.
  extern D3DPresent_t orig_present_fx;  // function pointer storage for D3D::Present.

  /**
   *
   */
  HRESULT WINAPI present(LPDIRECT3DDEVICE9, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);

  /**
   *
   */
  LRESULT CALLBACK wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param);
}

#endif
