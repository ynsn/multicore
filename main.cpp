#include "multicore/allocator.hpp"

#include <array>
#include <iostream>



auto main(int argc, char** argv) -> int {
  std::allocator<int> int_alloc;

auto p = mtc::allocate(int_alloc, 10);

  return 0;
}
