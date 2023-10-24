#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "allocator.h"
#include "haversine.h"

#define DEBUG_TIMERS_COUNT 1024
#include "metrics.h"


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

 INIT_TIMER();


  if (argc < 2) {
    fprintf(stderr,
            "expected exactly 1 arguments %d given: path to json file "
            "expected\n",
            argc);
    return EXIT_FAILURE;
  }
  const char *path = argv[1];
	  size_t read = 0;
	  int exit_code = EXIT_SUCCESS;
	  int start_of_double_literal = 0;
	  char *double_literal = NULL;
	  char *read_buffer = NULL;
	  size_t size = 0;
	  FILE *json = NULL;
{
	TIME_BLOCK("get size of json file");
	  json = fopen(path, "r");
	  if (json == NULL) {
	    fprintf(stderr, "failed to open the file\n");
	    return EXIT_FAILURE;
	  }
	  fseek(json, 0L, SEEK_END);
	  size = ftell(json);
	  rewind(json);
	END_TIMER();
}
{
	TIME_BLOCK_WITH_BANDWIDTH("read json file", size);
	  read_buffer = huge_malloc(size);
	  if(read_buffer == MAP_FAILED) {
		perror("failed to use the allocator: ");
		return EXIT_FAILURE;
	  }
	  fprintf(stderr, "size of the file is: %lu\n", size);
	  read = fread(read_buffer, 1, size, json);
	  fprintf(stderr, "we just read %lu bytes\n", read);
	  if (read != size) {
	    fprintf(stderr, "number of bytes read %lu is not expected, %lu expected\n",
	            read, size);
	    huge_free(read_buffer, size);
	    return EXIT_FAILURE;
	  }
	  fclose(json);
	END_TIMER();
}
  // parse it
 struct point_pair_array array;
 {
  TIME_BLOCK_WITH_BANDWIDTH("parsing", size);
	  array = array_make(4096);
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
	  }
  END_TIMER();
 }
 {
 TIME_BLOCK("free read buffer");
  huge_free(read_buffer, size);
 END_TIMER();
 }

  // compute avg
  {
          TIME_BLOCK("avg computing and output");
	  double avg;
	  avg = AvgDistance(array.data,  array.length);
	  {
	    TIME_BLOCK("free array of distances");
	    array_free(array);
	    END_TIMER();
	  }
	  {
	    TIME_BLOCK("prinf avg distance");
	    fprintf(stderr, "avg haversine destination based on loaded data: %f\n", avg);
	    END_TIMER();
	  }
          END_TIMER();

  }
  PRINT_TIMINGS();
  return EXIT_SUCCESS;
}
