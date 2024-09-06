#include <cassert>
#include <cmath>
#include <iostream>
#include <multicore.hpp>

struct test {
  bool await_ready() const noexcept { return true; }
        void await_suspend(std::coroutine_handle<> handle) const noexcept {}
  float await_resume() const noexcept { return 0.0f; }
};
auto main(int argc, char **argv) -> int {
  using w = mtc::await_result_t<test>;

  return 0;
}
