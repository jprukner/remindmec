#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "haversine.h"
#include "metrics.c"

// 32Ki
#define READ_BUFFER_SIZE 32 * 1024

enum token {
  UNEXPECTED_TOKEN = 0,
  EXPECTED_TOKEN = 1,
  START_OF_X0_AFTER_NEXT_TOKEN = 3,
  START_OF_X1_AFTER_NEXT_TOKEN = 4,
  START_OF_Y0_AFTER_NEXT_TOKEN = 5,
  START_OF_Y1_AFTER_NEXT_TOKEN = 6,
  END_OF_SECTION = 7,
};

// returns number greater than 0 that represents start of a new section in the
// json, 0 otherwise.
int is_expected(char first, char second, char third) {
  switch (first) {
  case '{':
    return second == '\n' || second == '"' || second == '{';
  case '\n':
    return second == '{' || second == ']' || second == '}';
  case '"':
    return second == 'x' || second == 'y' || second == 'p' || second == ':';
  case 'x':
    if (second == '0' && third == '"') {
      return START_OF_X0_AFTER_NEXT_TOKEN;
    } else if (second == '1' && third == '"') {
      return START_OF_X1_AFTER_NEXT_TOKEN;
    }
    return UNEXPECTED_TOKEN;
  case 'y':
    if (second == '0' && third == '"') {
      return START_OF_Y0_AFTER_NEXT_TOKEN;
    } else if (second == '1' && third == '"') {
      return START_OF_Y1_AFTER_NEXT_TOKEN;
    }
    return UNEXPECTED_TOKEN;
  case 'p':
    return second == 'a';
  case 'a':
    return second == 'i';
  case 'i':
    return second == 'r';
  case 'r':
    return second == 's';
  case 's':
    return (second == '"');
  case ':':
    return (second >= '0' && second <= '9') || second == '{' || second == '[' ||
           second == '-';
  case '-':
    return (second >= '0' && second <= '9');
  case '[':
    return second == '{' || second == '\n';
  case '0':
    return (second == '.') || (second >= '0' && second <= '9') ||
           second == ',' || second == '"' || second == '}';
  case '1':
    return (second == '.') || (second >= '0' && second <= '9') ||
           second == ',' || second == '"' || second == '}';
  case '2':
    return (second == '.') || (second >= '0' && second <= '9') ||
           second == ',' || second == '}';
  case '3':
    return (second == '.') || (second >= '0' && second <= '9') ||
           second == ',' || second == '}';
  case '4':
    return (second == '.') || (second >= '0' && second <= '9') ||
           second == ',' || second == '}';
  case '5':
    return (second == '.') || (second >= '0' && second <= '9') ||
           second == ',' || second == '}';
  case '6':
    return (second == '.') || (second >= '0' && second <= '9') ||
           second == ',' || second == '}';
  case '7':
    return (second == '.') || (second >= '0' && second <= '9') ||
           second == ',' || second == '}';
  case '8':
    return (second == '.') || (second >= '0' && second <= '9') ||
           second == ',' || second == '}';
  case '9':
    return (second == '.') || (second >= '0' && second <= '9') ||
           second == ',' || second == '}';
  case '.':
    return (second >= '0' && second <= '9');
  case ']':
    return second == '}' || second == '\n';
  case '}':
    if (second == ',' || second == '}' || second == '\n') {
      return END_OF_SECTION;
    } else if (second == '\0') {
      return EXPECTED_TOKEN;
    }
    return UNEXPECTED_TOKEN;
  case ',':
    if (second == ' ') {
      return END_OF_SECTION;
    } else if (second == '\n') {
      return EXPECTED_TOKEN;
    }
    return UNEXPECTED_TOKEN;
  case ' ':
    return second == '"';
  case '\0':
    return second == '\0';
  }
  return 0;
}

int main(int argc, char *argv[]) {
  uint64_t os_timer_start = read_os_timer();
  uint64_t cpu_timer_start = read_cpu_timer();
  if (argc < 2) {
    fprintf(stderr,
            "expected exactly 1 arguments %d given: path to json file "
            "expected\n",
            argc);
    return EXIT_FAILURE;
  }
  const char *path = argv[1];

  // read json file
  int exit_code = EXIT_SUCCESS;
  uint64_t file_reading_start = read_cpu_timer();
  FILE *json = fopen(path, "r");
  if (json == NULL) {
    fprintf(stderr, "failed to open the file\n");
    return EXIT_FAILURE;
  }
  size_t read = 0;
  int start_of_double_literal = 0;
  char *double_literal = NULL;
  fseek(json, 0L, SEEK_END);
  size_t size = ftell(json);
  rewind(json);
  fprintf(stderr, "size of the file is: %lu\n", size);
  char *read_buffer = malloc(size);
  read = fread(read_buffer, 1, size, json);
  fprintf(stderr, "we just read %lu bytes\n", read);
  if (read != size) {
    fprintf(stderr, "number of bytes read %lu is not expected, %lu expected\n",
            read, size);
    free(read_buffer);
    return EXIT_FAILURE;
  }
  fclose(json);
  uint64_t file_reading_stop = read_cpu_timer();
  uint64_t file_reading_elapsed = file_reading_stop - file_reading_start;

  // parse it
  uint64_t file_parsing_start = read_cpu_timer();
  struct point_pair_array array = array_make(512);
  char first;
  char second;
  char third;
  struct point_pair pair;
  double *current_field;
  uint64_t field_counter = 0;
  for (uint64_t n = 0; n < read; ++n) {
    first = read_buffer[n];
    if (n < read - 1) {
      second = read_buffer[n + 1];
    } else {
      second = '\0';
    }

    if (n < read - 2) {
      third = read_buffer[n + 2];
    } else {
      third = '\0';
    }

    int expected = is_expected(first, second, third);
    if (expected == 0) {
      fprintf(stderr,
              "\nunexpected character in the following sequence `%c%c%c`\n",
              first, second, third);
      return EXIT_FAILURE;
    } else if (expected == START_OF_X0_AFTER_NEXT_TOKEN) {
      start_of_double_literal = n + 4;
      current_field = &(pair.x0);
    } else if (expected == START_OF_X1_AFTER_NEXT_TOKEN) {
      start_of_double_literal = n + 4;
      current_field = &(pair.x1);
    } else if (expected == START_OF_Y0_AFTER_NEXT_TOKEN) {
      start_of_double_literal = n + 4;
      current_field = &(pair.y0);
    } else if (expected == START_OF_Y1_AFTER_NEXT_TOKEN) {
      start_of_double_literal = n + 4;
      current_field = &(pair.y1);
    } else if (expected == END_OF_SECTION) {
      read_buffer[n] = '\0'; // terminate the string, the character was read
                             // anyway so we can do that
      // this way we can construct a literal
      // without allocating memory
      double_literal = read_buffer + start_of_double_literal;
      double number = atof(double_literal);
      *current_field = number;
      field_counter += 1;
    }
    if (field_counter == 4) {
      field_counter = 0;
      array = array_append(array, pair);
      pair.x0 = 0;
      pair.x1 = 0;
      pair.y0 = 0;
      pair.y1 = 0;
    }
    // putchar(first);
  }
  // for (int n = 0; n < array.length; ++n) {
  //   fprintf(stderr, "x0: %f, y0: %f, x1: %f, y1: %f\n", array.data[n].x0,
  //           array.data[n].y0, array.data[n].x1, array.data[n].y1);
  // }
  free(read_buffer);
  uint64_t file_parsing_stop = read_cpu_timer();
  uint64_t file_parsing_elapsed = file_parsing_stop - file_parsing_start;

  // compute avg
  uint64_t distance_computing_start = read_cpu_timer();
  double *distances = malloc(sizeof(double) * array.length);
  double avg = AvgDistance(array.data, distances, array.length);
  array_free(array);
  fprintf(stderr, "avg haversine destination based on loaded data: %f\n", avg);
  uint64_t distance_computing_stop = read_cpu_timer();
  uint64_t distance_computing_elapsed =
      distance_computing_stop - distance_computing_start;

  uint64_t os_timer_stop = read_os_timer();
  uint64_t os_timer_elapsed = os_timer_stop - os_timer_start;

  uint64_t cpu_timer_stop = read_cpu_timer();
  uint64_t cpu_timer_elapsaed = cpu_timer_stop - cpu_timer_start;

  fprintf(stderr, "Time elapsed: %.4fs\n",
          (double)os_timer_elapsed / (double)OS_TIMER_FREQUENCY);

  fprintf(stderr, "\tfile reading: %lu (%.2f%%)\n", file_reading_elapsed,
          ((double)file_reading_elapsed / (double)cpu_timer_elapsaed) * 100);
  fprintf(stderr, "\tfile parsing: %lu (%.2f%%)\n", file_parsing_elapsed,
          ((double)file_parsing_elapsed / (double)cpu_timer_elapsaed) * 100);
  fprintf(stderr, "\thaversine distance computing: %lu (%.2f%%)\n",
          distance_computing_elapsed,
          ((double)distance_computing_elapsed / (double)cpu_timer_elapsaed) *
              100);
  return EXIT_SUCCESS;
}
