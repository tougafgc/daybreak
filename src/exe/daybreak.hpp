/**
 * daybreak.hpp
 * By: tougafgc
 * Date: 22 November 2025
 *
 * Methods pertaining to recognizing
 * and injecting the DLL into Melty Blood.
 **/
#ifndef DAYBREAK_HPP_
#define DAYBREAK_HPP_

#include "../lib.hpp"

#define MELTY_NOT_FOUND    0
#define MELTY_FOUND        1
#define MELTY_PROGRAM_NAME "MBAA.exe"

class Daybreak {
  public:
    Daybreak();
    ~Daybreak() = default;

    // Find an MBAA.exe process and inject Daybreak
    void hook(std::string dll_path);

  private:
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    // Take a snapshot of the current open processes
    // and find the PID of Melty
    int  find_melty();

    // Open Melty
    void spawn_process(const char *path);

    void inject(std::string dll_path);

    // ASM injection that skips the configuration menu
    // for Melty
    void skip_config();
};

#endif // DAYBREAK_HPP_
