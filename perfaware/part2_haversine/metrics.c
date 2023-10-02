#include <stdint.h>
#include <sys/time.h>
#include <x86intrin.h>

#define OS_TIMER_FREQUENCY 1000000

uint64_t read_os_timer() {
  struct timeval value;
  gettimeofday(&value, 0);

  uint64_t result =
      OS_TIMER_FREQUENCY * (uint64_t)value.tv_sec + (uint64_t)value.tv_usec;
  return result;
}

inline uint64_t read_cpu_timer() { return __rdtsc(); }
