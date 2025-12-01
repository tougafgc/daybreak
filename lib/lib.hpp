/**
 * main.cpp
 * By: tougafgc
 * Date: 1 December 2025
 *
 * Central place for universal functions,
 * includes, and any constants to avoid 
 * magic numbers.
 **/
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
#define DAYBREAK_ERROR 1
#define IMGUI_HIDE false
#define IMGUI_SHOW true

inline void gui_error(std::string message) {
  MessageBox(NULL, message.c_str(), "[ERROR]", MB_OK);
  exit(DAYBREAK_ERROR);
}

inline void gui_info(std::string message) {
  MessageBox(NULL, message.c_str(), "[INFO]", MB_OK);
}

inline void txt_error(std::string message) {
  std::cerr << message << std::endl;
}

inline void txt_info(std::string message) {
  std::cout << message << std::endl;
}

inline void debug_msg(std::string msg) {
  std::cout << "[DEBUG] " << msg << std::endl;
}

#endif // LIB_HPP_
