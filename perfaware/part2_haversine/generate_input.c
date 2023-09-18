#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#define CLUSTER_RANGE 60
#define EARTH_RADIUS 6372.8

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

int16_t random_in_range(int16_t lower, int16_t upper) {
  return (rand() % (upper - lower + 1)) + lower;
}

struct point {
  int16_t x;
  int16_t y;
};

int main(int argc, char *argv[]) {

  if (argc < 3) {
    fprintf(stderr,
            "expected exactly 2 arguments %d given: cluster count and points "
            "count per cluster expected\n",
            argc);
    return EXIT_FAILURE;
  }

  // TODO check for errors;
  int64_t cluster_count = atol(argv[1]);
  int64_t points_per_cluster = atol(argv[2]);
  if (cluster_count <= 0 || points_per_cluster <= 0) {
    fprintf(stderr, "cluster count and points per cluster need to be greater "
                    "than zero\n");
    return EXIT_FAILURE;
  }

  fprintf(stderr, "cluster_count: %ld\npoints_per_cluster:%ld\n", cluster_count,
          points_per_cluster);

  uint64_t total_number_of_points = cluster_count * points_per_cluster;
  struct point *points = malloc(sizeof(struct point) * total_number_of_points);

  for (int n = 0; n < cluster_count; ++n) {

    uint64_t points_count = 0;
    int16_t cluster_centre_x = random_in_range(-180, 180);
    int16_t cluster_centre_y = random_in_range(-90, 90);
    while (points_count < points_per_cluster) {
      int16_t new_point_x_start = cluster_centre_x - (CLUSTER_RANGE / 2);

      if (new_point_x_start < -180) {
        new_point_x_start = -180;
      }
      int16_t new_point_x_stop = cluster_centre_x + (CLUSTER_RANGE / 2);
      if (new_point_x_stop > 180) {
        new_point_x_stop = 180;
      }

      int16_t new_point_y_start = cluster_centre_y - (CLUSTER_RANGE / 2);
      if (new_point_y_start < -90) {
        new_point_y_start = -90;
      }

      int16_t new_point_y_stop = cluster_centre_y + (CLUSTER_RANGE / 2);
      if (new_point_y_stop > 90) {
        new_point_y_stop = 90;
      }

      int16_t new_point_x =
          random_in_range(new_point_x_start, new_point_x_stop);

      int16_t new_point_y =
          random_in_range(new_point_y_start, new_point_y_stop);

      struct point point = {.x = new_point_x, .y = new_point_y};
      points[(n * points_per_cluster) + points_count] = point;

      points_count += 1;
    }
  }

  // compute distances
  uint64_t total_count_of_distances =
      (total_number_of_points * (total_number_of_points - 1)) / 2;
  double *distances = malloc(sizeof(f64) * total_count_of_distances);
  printf("distances: %p\n", distances);
  uint64_t distances_count = 0;
  for (uint64_t a = 0; a < total_number_of_points - 1; ++a) {
    for (uint64_t b = a + 1; b < total_number_of_points; ++b) {
      struct point point_a = points[a];
      struct point point_b = points[b];
      distances[distances_count] = ReferenceHaversine(
          point_a.x, point_a.y, point_b.x, point_b.y, EARTH_RADIUS);
      distances_count += 1;
    }
  }

  double sum = 0;
  for (uint64_t n = 0; n < total_count_of_distances; ++n) {
    sum += distances[n];
  }

  printf("avg distance: %f/%lu = %f\n", sum, total_count_of_distances,
         sum / total_count_of_distances);

  free(distances);
  free(points);
}