/**
 * main.cpp
 * By: tougafgc
 * Date: 22 November 2025
 *
 * Entry point into Daybreak. One click
 * is all that's needed to start Melty Blood,
 * hook into it, and make endless modifications.
 **/
#include "exe/daybreak.hpp"
#include "lib.hpp"

int main(int argc, char *argv[]) {
  std::filesystem::path pwd = std::filesystem::current_path();
  std::filesystem::path full_path = pwd / "daybreak\\daybreak.dll";

  Daybreak daybreak;
  if (std::filesystem::exists(full_path)) {
    debug_msg(full_path.string());
    daybreak.hook(full_path.string());
  } else {
    printf("%s\ndoes not exist!\n\n", full_path.string().c_str());
  }

  return 0;
}
