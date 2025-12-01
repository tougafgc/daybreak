/**
 * main.cpp
 * By: tougafgc
 * Date: 22 November 2025
 *
 * Entry point into Daybreak. One click
 * is all that's needed to start Melty Blood,
 * hook into it, and make endless modifications.
 **/
#include "lib/lib.hpp"
#include "exe/daybreak.hpp"

int main(int argc, char *argv[]) {
  std::filesystem::path pwd = std::filesystem::current_path();
  std::filesystem::path full_path = pwd / "daybreak\\crescent.dll";

  Daybreak daybreak;
  if (std::filesystem::exists(full_path)) {
    debug_msg(full_path.string());
    daybreak.hook(full_path.string());
  } else {
    printf("crescent.dll\ncannot be found!\n\n");
  }

  return 0;
}
