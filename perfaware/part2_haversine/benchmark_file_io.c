#include "benchmarks.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define FILE_NAME "pairs.json"

void benchmark_fread(struct benchmark *benchmark, size_t size) {
  FILE *file = fopen(FILE_NAME, "r");
  if (file == NULL) {
    printf("opening of the file failed\n");
    return;
  }

  char *buffer = malloc(size);
  int read = 0;
  while (is_running(benchmark)) {
    start(benchmark);
    read = fread(buffer, size, 1, file);
    if (read != 1) {
      perror("failed to read file");
      fail(benchmark, "failed to read file");
    }
    stop(benchmark);
    rewind(file);
  }
  free(buffer);
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