#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <x86intrin.h>

#define OS_TIMER_FREQUENCY 1000000
#define MEGA_BYTE (1024.0f * 1024.0f)
#define GIGA_BYTE (MEGA_BYTE * 1024.0f)

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
  size_t bytes_processed;
};

struct profiler {
  struct timing *timings;
  uint64_t start;
  uint64_t start_os_time;
  int64_t timings_counter;
};

uint64_t estimate_block_timer_frequency(void) {

  uint64_t MillisecondsToWait = 100;
  uint64_t OSFreq = OS_TIMER_FREQUENCY;

  uint64_t BlockStart = read_cpu_timer();
  uint64_t OSStart = read_os_timer();
  uint64_t OSEnd = 0;
  uint64_t OSElapsed = 0;
  uint64_t OSWaitTime = OSFreq * MillisecondsToWait / 1000;
  while (OSElapsed < OSWaitTime) {
    OSEnd = read_os_timer();
    OSElapsed = OSEnd - OSStart;
  }

  uint64_t BlockEnd = read_cpu_timer();
  uint64_t BlockElapsed = BlockEnd - BlockStart;

  uint64_t BlockFreq = 0;
  if (OSElapsed) {
    BlockFreq = OSFreq * BlockElapsed / OSElapsed;
  }

  return BlockFreq;
}

static struct profiler _global_profiler;

#ifdef DEBUG_TIMERS_COUNT

#define INIT_TIMER()                                                           \
  do {                                                                         \
    _global_profiler.timings =                                                 \
        calloc(DEBUG_TIMERS_COUNT, sizeof(struct timing));                     \
    _global_profiler.timings_counter = -1;                                     \
    _global_profiler.start_os_time = read_os_timer();                          \
    _global_profiler.start = read_cpu_timer();                                 \
  } while (0)

#define TIME_BLOCK_WITH_BANDWIDTH(block_name, size)                            \
  size_t data_size = size;                                                     \
  uint64_t _start = read_cpu_timer();                                          \
  int64_t origin = _global_profiler.timings_counter;                           \
  const char *label = block_name;

#define TIME_BLOCK(block_name) TIME_BLOCK_WITH_BANDWIDTH(block_name, 0)

#define TIME_FUNCTION TIME_BLOCK(__func__)

#define TIME_FUNCTION_WITH_BANDWIDTH(size)                                     \
  TIME_BLOCK_WITH_BANDWIDTH(__func__, size)

#define END_TIMER()                                                            \
  do {                                                                         \
    uint64_t _stop = read_cpu_timer();                                         \
    struct timing _timing = {0};                                               \
    _timing.elapsed = _stop - _start;                                          \
    _timing.label = label;                                                     \
    _timing.bytes_processed = data_size;                                       \
    uint64_t elapsed_in_nested_blocks = 0;                                     \
    for (int64_t i = _global_profiler.timings_counter; i > origin; i--) {      \
      struct timing nested_timing = _global_profiler.timings[i];               \
      elapsed_in_nested_blocks += nested_timing.elapsed;                       \
    }                                                                          \
    _timing.elapsed -= elapsed_in_nested_blocks;                               \
    _global_profiler.timings_counter++;                                        \
    _global_profiler.timings[_global_profiler.timings_counter] = _timing;      \
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
    uint64_t block_frequency = estimate_block_timer_frequency();               \
    double elapsed_os_time_total = (double)_elapsed_total / (double)block_frequency; \
    printf("\nTotal seconds elapsed: %f\n", elapsed_os_time_total);            \
    for (uint64_t i = 0; i <= _global_profiler.timings_counter; ++i) {         \
      struct timing _timing = _global_profiler.timings[i];                     \
      printf("\t%s: %lu (%.2f %%)", _timing.label, _timing.elapsed,            \
             100 * ((double)_timing.elapsed / (double)_elapsed_total));        \
      if (_timing.bytes_processed) {                                           \
        double seconds =                                                       \
            (double)_timing.elapsed / (double)block_frequency;                 \
        double bytes_per_second = (double) _timing.bytes_processed / seconds;  \
        double mega_bytes_processed =                                          \
            (double)_timing.bytes_processed / (double)MEGA_BYTE;               \
        double speed = bytes_per_second / GIGA_BYTE;                           \
        printf(" %f MB processed at %f GB/s", mega_bytes_processed, speed);    \
      }                                                                        \
      printf("\n");                                                            \
    }                                                                          \
    free(_global_profiler.timings);                                            \
  } while (0)

#else

#define INIT_TIMER()
#define TIME_FUNCTION
#define TIME_BLOCK(label)
#define END_TIMER()
#define RETURN(x) return x;
#define RETURN_NOTHING return;
#define PRINT_TIMINGS()

#endif
