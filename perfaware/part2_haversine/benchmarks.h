#include "metrics.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#define MAX_RUN_TIME_SECONDS 10

enum status {
  RUNNING = 1,
  DONE = 2,
  ERROR = 3,
};

struct benchmark {
  uint64_t start_cpu_time, start_os_time;
  uint64_t elapsed_min;
  enum status status;
  const char *error;
};

void start(struct benchmark *benchmark) {
  benchmark->start_cpu_time = read_cpu_timer();
  return;
}

void stop(struct benchmark *benchmark) {
  if (benchmark->status == ERROR) {
    // don't record this run as it may be too fast if there is an error;
    return;
  }
  uint64_t elapsed = read_cpu_timer() - benchmark->start_cpu_time;
  if (elapsed < benchmark->elapsed_min) {
    benchmark->start_os_time = read_os_timer();
    benchmark->elapsed_min = elapsed;
    printf("fastest run is now: %lu\n", elapsed);
  }
  double elapsed_seconds =
      (double)(read_os_timer() - benchmark->start_os_time) /
      (double)OS_TIMER_FREQUENCY;
  if (elapsed_seconds > MAX_RUN_TIME_SECONDS) {
    benchmark->status = DONE;
  }
}

uint64_t is_running(struct benchmark *benchmark) {
  if (benchmark->status == ERROR) {
    printf("DONE WITH ERROR: %s\n", benchmark->error);
  }
  return benchmark->status == RUNNING;
}

void run_benchmark(void (*func)(struct benchmark *benchmark, size_t size),
                   size_t bytes_processed) {
  struct benchmark benchmark = {0};
  benchmark.start_os_time = read_os_timer();
  benchmark.status = RUNNING;
  benchmark.elapsed_min = UINT64_MAX;
  func(&benchmark, bytes_processed);
  printf("fastest run: %lu", benchmark.elapsed_min);
  if (bytes_processed) {
    uint64_t block_frequency = estimate_block_timer_frequency();
    double mb = (double)bytes_processed / (double)MEGA_BYTE;
    double seconds = (double)benchmark.elapsed_min / (double)block_frequency;
    double gps = ((double)bytes_processed / (double)GIGA_BYTE) / seconds;
    printf(" and it processed %f MB of data at speed %f GB/s", mb, gps);
  }
  printf("\n");
}

void fail(struct benchmark *benchmark, const char *error) {
  benchmark->status = ERROR;
  benchmark->error = error;
}