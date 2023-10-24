#define _GNU_SOURCE

#include "benchmarks.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "allocator.h"

#define FILE_NAME "pairs.json"
#define DO_ALLOCATION_EVERY_TIME 1

void benchmark_fread(struct benchmark *benchmark, size_t size) {
  FILE *file = fopen(FILE_NAME, "r");
  if (file == NULL) {
    printf("opening of the file failed\n");
    return;
  }

  int read = 0;
  char *buffer = NULL;
  while (is_running(benchmark)) {
    if(buffer == NULL || DO_ALLOCATION_EVERY_TIME) {
        buffer = huge_malloc(size);
    }
    start(benchmark);
    read = fread(buffer, size, 1, file);
    stop(benchmark);
    if (read != 1) {
      perror("failed to read file");
      fail(benchmark, "failed to read file");
    }
    rewind(file);
    if(DO_ALLOCATION_EVERY_TIME) {
       if (huge_free(buffer, size) < 0) {
	perror("failed to free: ");
	}
    }
  }
  if(!DO_ALLOCATION_EVERY_TIME) {
       if (huge_free(buffer, size) < 0) {
	perror("failed to free: ");
	}

  }
  fclose(file);
}

int main() {

  size_t size;
  struct stat st;
  if (stat(FILE_NAME, &st) != 0) {
    printf("stat failed\n");
    return EXIT_FAILURE;
  }
  size = st.st_size;

  run_benchmark(benchmark_fread, size);
}
