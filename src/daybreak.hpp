#ifndef MELTY_SCHEME_H_
#define MELTY_SCHEME_H_

#include <cstdio>
#include <iostream>
#include <string>
#include <filesystem>
#include <windows.h>
#include <tlhelp32.h>

#define DLL_DEBUG 1

#define MELTY_NOT_FOUND    0
#define MELTY_FOUND        1
#define MELTY_PROGRAM_NAME "MBAA.exe"

class Daybreak {
  public:
    enum ProcessType {
      NO_ARGS   = 0,
      WITH_ARGS = 1
    };

    Daybreak();
    ~Daybreak() = default;

    void spawn_process(const char *path, ProcessType type);
    void debug_msg(std::string messagee);
    void die(std::string msg);
    int  find_melty();
    void inject(std::string dll_path);
};

#endif // MELTY_SCHEME_H_
