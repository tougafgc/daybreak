#ifndef LIB_HPP_
#define LIB_HPP_

// Windows
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <d3d9.h>

// CPP
#include <string>
#include <iostream>
#include <filesystem>

// C
#include <cstdint>
#include <cstdio>

// Defines
#define DAYBREAK_DEBUG 1

inline void gui_error(std::string message) {
  MessageBox(NULL, message.c_str(), "Error!", MB_OK);
  exit(1);
}

inline void gui_info(std::string message) {
  MessageBox(NULL, message.c_str(), "[INFO]", MB_OK);
}

inline void txt_error(std::string message) {
  fprintf(stderr, message.c_str());
}

inline void txt_info(std::string message) {
  printf("%s\n", message.c_str());
}

inline void debug_msg(std::string msg) {
  if (DAYBREAK_DEBUG) printf("[DEBUG] %s\n", msg.c_str());
}

#endif // LIB_HPP_
