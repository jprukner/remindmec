#include <stdint.h>
#include <stdlib.h>

struct point_pair {
  double x0;
  double y0;
  double x1;
  double y1;
};

struct point_pair_array {
  uint64_t length;
  uint64_t capacity;
  struct point_pair *data;
};

struct point_pair_array array_make(uint64_t capacity) {
  struct point_pair_array array;
  array.length = 0;
  if (capacity != 0) {
    array.capacity = capacity * 2;
  } else {
    array.capacity = 64;
  }
  array.data =
      (struct point_pair *)malloc(array.capacity * sizeof(struct point_pair));
  return array;
}

struct point_pair_array array_append(struct point_pair_array array,
                                     struct point_pair pair) {
  if (array.length + 1 >= array.capacity) {
    array.capacity = array.capacity * 2;
    array.data = (struct point_pair *)realloc(
        (void *)array.data, array.capacity * sizeof(struct point_pair));
  }
  array.data[array.length] = pair;
  array.length += 1;
  return array;
}

void array_free(struct point_pair_array array) { free(array.data); }