#define DEBUG_TIMERS_COUNT 100
#include "metrics.h"

int multiplyNumbers(int n) {
  TIME_FUNCTION;
  if (n >= 1)
    return n * multiplyNumbers(n - 1);
  else
    RETURN(1);
}

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
  {
    TIME_BLOCK("d: while 1000 counts down to 0");
    int b = 1000;
    while (b != 0) {
      b--;
    }
    END_TIMER();
  }
  RETURN_NOTHING();
}

void recursive_int(int count) {
  TIME_FUNCTION;
  if (count <= 0) {
    RETURN_NOTHING();
  }
  int a = 100;
  while (a != 0) {
    a--;
  }
  recursive_int(count - 1);
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
  recursive_int(5);
  PRINT_TIMINGS();
}
