#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include "allocator.h"

#define EARTH_RADIUS 6372.8

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
      (struct point_pair *)huge_malloc(array.capacity * sizeof(struct point_pair));
  if (array.data == MAP_FAILED) {
	perror("failed to create an array: ");
  }
  return array;
}

struct point_pair_array array_append(struct point_pair_array array,
                                     struct point_pair pair) {
  if (array.length + 1 >= array.capacity) {
    size_t old_capacity = array.capacity;
    array.capacity = array.capacity * 2;
    array.data = (struct point_pair *)huge_realloc(
        (void *)array.data, old_capacity*sizeof(struct point_pair), array.capacity * sizeof(struct point_pair));
    if (array.data == MAP_FAILED) {
          perror("failed to scale the array: ");
    }
  }
  array.data[array.length] = pair;
  array.length += 1;
  return array;
}

int array_free(struct point_pair_array array) { huge_free(array.data, array.capacity * sizeof(struct point_pair)); }

typedef double f64;

static f64 Square(f64 A) {
  f64 Result = (A * A);
  return Result;
}

static f64 RadiansFromDegrees(f64 Degrees) {
  f64 Result = 0.01745329251994329577 * Degrees;
  return Result;
}

static f64 ReferenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1, f64 EarthRadius) {

  f64 lat1 = Y0;
  f64 lat2 = Y1;
  f64 lon1 = X0;
  f64 lon2 = X1;

  f64 dLat = RadiansFromDegrees(lat2 - lat1);
  f64 dLon = RadiansFromDegrees(lon2 - lon1);
  lat1 = RadiansFromDegrees(lat1);
  lat2 = RadiansFromDegrees(lat2);

  f64 a =
      Square(sin(dLat / 2.0)) + cos(lat1) * cos(lat2) * Square(sin(dLon / 2));
  f64 c = 2.0 * asin(sqrt(a));

  f64 Result = EarthRadius * c;

  return Result;
}

f64 AvgDistance(struct point_pair array[], uint64_t length) {

  double sum = 0;
  for (uint64_t a = 0; a < length; ++a) {
    struct point_pair pair = array[a];
    double distance =
        ReferenceHaversine(pair.x0, pair.y0, pair.x1, pair.y1, EARTH_RADIUS);
    sum += distance;
  }
  // compute average distance
  f64 average_distance = sum / length;
  fprintf(stderr, "avg distance: %f/%lu = %f\n", sum, length, average_distance);

  return average_distance;
}
