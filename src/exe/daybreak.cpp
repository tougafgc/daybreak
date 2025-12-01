#include "daybreak.hpp"

Daybreak::Daybreak() {
  memset(&this->si, 0, sizeof(this->si));
  memset(&this->pi, 0, sizeof(this->pi));
  this->si.cb = sizeof(this->si);
}

int Daybreak::find_melty() {
  HANDLE proc_snapshot;
  PROCESSENTRY32  proc_melty;
  BOOL proc_found;
  int pid = MELTY_NOT_FOUND;

  proc_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (proc_snapshot == INVALID_HANDLE_VALUE)
    return MELTY_NOT_FOUND;

  // Take a snapshot of current processes,
  // iterate through each item and compare
  // the names of the processes to MBAA.exe.
  // Return the PID if found.
  proc_melty.dwSize = sizeof(PROCESSENTRY32);
  proc_found = Process32First(proc_snapshot, &proc_melty);
  while (proc_found) {
    if (strcmp(MELTY_PROGRAM_NAME, proc_melty.szExeFile) == 0) {
      pid = proc_melty.th32ProcessID;
      break;
    }

    proc_found = Process32Next(proc_snapshot, &proc_melty);
  }

  CloseHandle(proc_snapshot);
  return pid;
}

void Daybreak::hook(std::string dll_path) {
  spawn_process("MBAA.exe");
  inject(dll_path);
  skip_config();

  ResumeThread(this->pi.hThread);
}

void Daybreak::spawn_process(const char *path) {
  BOOL res = FALSE;

  // MBAA.exe needs to be created on a suspended thread,
  // so the skip_config injection will function correctly.
  res = CreateProcessA((LPSTR)path, NULL, NULL, NULL, TRUE,
                       CREATE_SUSPENDED, NULL, NULL, &this->si, &this->pi);

  if (!res) {
    gui_error("Could not start " + std::string(path) + "\n");
  }
}

void Daybreak::inject(std::string dll_path) {
  LPTHREAD_START_ROUTINE loadlib_fx = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
  std::wstring w_dll_path = std::wstring(dll_path.begin(), dll_path.end());
  size_t w_dll_len = (w_dll_path.length() + 1) * sizeof(wchar_t);
  int pid = MELTY_NOT_FOUND;
  HANDLE melty_thread;
  LPVOID buffer;

  debug_msg("Finding Melty...");
  while (!pid) pid = find_melty();
  debug_msg("Melty PID: " + std::to_string(pid));

  buffer = VirtualAllocEx(this->pi.hProcess, NULL, w_dll_len, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  WriteProcessMemory(this->pi.hProcess, buffer, w_dll_path.c_str(), w_dll_len, NULL);

  DWORD tid;
  melty_thread = CreateRemoteThread(this->pi.hProcess, 0, 0, loadlib_fx, buffer, 0, &tid);

  WaitForSingleObject(melty_thread, INFINITE);

  debug_msg("hProcess: " + std::to_string((int)this->pi.hProcess));
  debug_msg("buffer: " + std::to_string((int)buffer) + "\n");
  CloseHandle(melty_thread);
}

void Daybreak::skip_config() {
  // ASM hack that rewrites a jump call, skipping the configuration menu.
  // Provided from CCCaster, https://github.com/Rhekar/CCCaster/blob/master/tools/Launcher.cpp#L171 
  static const char jmp1[]  = { (char)0xEB, (char)0x0E };
  static const char jmp2[] = { (char)0xEB };

  WriteProcessMemory(this->pi.hProcess, (void*)0x004A1D42, jmp1, sizeof(jmp1), 0);
  WriteProcessMemory(this->pi.hProcess, (void*)(0x004A1D42 + 0x08), jmp2, sizeof(jmp2), 0);
}

