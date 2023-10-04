#define DEBUG_TIMERS_COUNT 100
#include "metrics.h"
void foo() {
  TIME_FUNCTION;
  int a = 1000;
  while (a != 0) {
    a--;
  }
  {
    TIME_BLOCK("c: while 1000 counts down to 0");
    int b = 1000;
    while (b != 0) {
      b--;
    }
    END_TIMER();
  }
  RETURN_NOTHING();
}

int main() {
  INIT_TIMER();
  int a = 10;
  TIME_BLOCK("fake timer");
  END_TIMER();
  {
    TIME_BLOCK("b: while 1000 counts down to 0");
    int b = 1000;
    while (b != 0) {
      b--;
    }
    END_TIMER();
  }
  foo();
  PRINT_TIMINGS();
}
