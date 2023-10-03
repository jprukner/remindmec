#include <assert.h>
#include <stdint.h>
#include <stdio.h>
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

uint64_t read_cpu_timer() { return __rdtsc(); }

struct timing {
  const char *label;
  uint64_t elapsed;
};

struct profiler {
  struct timing *timings;
  uint64_t start;
  uint64_t timings_counter;
};

static struct profiler _global_profiler;

#ifdef DEBUG_TIMERS_COUNT

#define INIT_TIMER()                                                           \
  do {                                                                         \
    _global_profiler.timings =                                                 \
        malloc(DEBUG_TIMERS_COUNT * sizeof(struct timing));                    \
    _global_profiler.start = read_cpu_timer();                                 \
    _global_profiler.timings_counter = 0;                                      \
  } while (0)

#define TIME_BLOCK(block_name)                                                 \
  uint64_t _start = read_cpu_timer();                                          \
  const char *label = block_name;

#define TIME_FUNCTION TIME_BLOCK(__func__)

#define END_TIMER()                                                            \
  do {                                                                         \
    struct timing _timing = {0};                                               \
    _timing.elapsed = read_cpu_timer() - _start;                               \
    _timing.label = label;                                                     \
    assert(_global_profiler.timings_counter < DEBUG_TIMERS_COUNT);             \
    _global_profiler.timings[_global_profiler.timings_counter] = _timing;      \
    _global_profiler.timings_counter++;                                        \
  } while (0)

#define RETURN_NOTHING()                                                       \
  do {                                                                         \
    END_TIMER();                                                               \
    return;                                                                    \
  } while (0)

#define RETURN(x)                                                              \
  do {                                                                         \
    END_TIMER();                                                               \
    return x;                                                                  \
  } while (0)

#define PRINT_TIMINGS()                                                        \
  do {                                                                         \
    uint64_t _elapsed_total = read_cpu_timer() - _global_profiler.start;       \
    for (uint64_t i = 0; i < _global_profiler.timings_counter; ++i) {          \
      struct timing _timing = _global_profiler.timings[i];                     \
      printf("%s: %lu (%.2f %%)\n", _timing.label, _timing.elapsed,            \
             100 * ((double)_timing.elapsed / (double)_elapsed_total));        \
    }                                                                          \
    free(_global_profiler.timings);                                            \
  } while (0)

#else

#define INIT_TIMER ;
#define TIME_IT ;
#define RETURN(x) return x;
#define RETURN_NOTHING return;

#endif
