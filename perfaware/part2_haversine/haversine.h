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

inline struct point_pair_array array_make(uint64_t length) {
  struct point_pair_array array;
  array.length = length;
  if (length != 0) {
    array.capacity = length * 2;
  } else {
    array.capacity = 64;
  }
  array.data =
      (struct point_pair *)malloc(array.capacity * sizeof(struct point_pair));
  return array;
}

inline struct point_pair_array array_append(struct point_pair_array array,
                                            struct point_pair pair) {
  if (array.length + 1 >= array.capacity) {
    array.capacity = array.capacity * 2;
    array.data =
        (struct point_pair *)realloc((void *)array.data, array.capacity);
  }
  array.data[array.length] = pair;
  array.length += 1;
  return array;
}

inline void array_free(struct point_pair_array array) { free(array.data); }