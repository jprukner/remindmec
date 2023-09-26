#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "haversine.h"

#define CLUSTER_RANGE 6000

int16_t random_in_range(int16_t lower, int16_t upper) {
  return (rand() % (upper - lower + 1)) + lower;
}

int main(int argc, char *argv[]) {

  if (argc < 3) {
    fprintf(
        stderr,
        "expected exactly 2 arguments %d given: cluster count and point pairs "
        "count per cluster expected\n",
        argc);
    return EXIT_FAILURE;
  }

  // TODO check for errors;
  int64_t cluster_count = atol(argv[1]);
  int64_t point_pairs_per_cluster = atol(argv[2]);
  if (cluster_count <= 0 || point_pairs_per_cluster <= 0) {
    fprintf(stderr,
            "cluster count and point pairs per cluster need to be greater "
            "than zero\n");
    return EXIT_FAILURE;
  }

  fprintf(stderr, "cluster_count: %ld\npoint_pairs_per_cluster:%ld\n",
          cluster_count, point_pairs_per_cluster);

  uint64_t total_number_of_pairs = cluster_count * point_pairs_per_cluster;
  fprintf(stderr, "generating total number of point pairs: %lu\n",
          total_number_of_pairs);
  struct point_pair *point_pairs =
      malloc(sizeof(struct point_pair) * total_number_of_pairs);

  for (int n = 0; n < cluster_count; ++n) {

    uint64_t point_pairs_count = 0;
    int16_t cluster_centre_x = random_in_range(-18000, 18000);
    int16_t cluster_centre_y = random_in_range(-9000, 9000);
    while (point_pairs_count < point_pairs_per_cluster) {
      int16_t new_point_x_start = cluster_centre_x - (CLUSTER_RANGE / 2);

      if (new_point_x_start < -18000) {
        new_point_x_start = -18000;
      }
      int16_t new_point_x_stop = cluster_centre_x + (CLUSTER_RANGE / 2);
      if (new_point_x_stop > 18000) {
        new_point_x_stop = 18000;
      }

      int16_t new_point_y_start = cluster_centre_y - (CLUSTER_RANGE / 2);
      if (new_point_y_start < -9000) {
        new_point_y_start = -9000;
      }

      int16_t new_point_y_stop = cluster_centre_y + (CLUSTER_RANGE / 2);
      if (new_point_y_stop > 9000) {
        new_point_y_stop = 9000;
      }

      int16_t new_point_x0 =
          random_in_range(new_point_x_start, new_point_x_stop);

      int16_t new_point_y0 =
          random_in_range(new_point_y_start, new_point_y_stop);

      int16_t new_point_x1 =
          random_in_range(new_point_x_start, new_point_x_stop);

      int16_t new_point_y1 =
          random_in_range(new_point_y_start, new_point_y_stop);

      struct point_pair pair = {
          .x0 = ((double)new_point_x0) / 100,
          .y0 = ((double)new_point_y0) / 100,
          .x1 = ((double)new_point_x1) / 100,
          .y1 = ((double)new_point_y1) / 100,
      };
      point_pairs[(n * point_pairs_per_cluster) + point_pairs_count] = pair;

      point_pairs_count += 1;
    }
  }

  // compute distances
  f64 *distances = malloc(sizeof(f64) * total_number_of_pairs);
  f64 average_distance =
      AvgDistance(point_pairs, distances, total_number_of_pairs);
  fprintf(stderr, "avg distance: %f\n", average_distance);

  int exit_code = EXIT_SUCCESS;
  {
    // write input json file
    FILE *data = fopen("pairs.json", "w");
    if (data == NULL) {
      exit_code = EXIT_FAILURE;
      fprintf(stderr, "failed to open the file");
      goto exit;
    }

    fprintf(data, "{\"pairs\":[");
    for (uint64_t a = 0; a < total_number_of_pairs; ++a) {
      struct point_pair pair = point_pairs[a];
      fprintf(data, "{\"x0\":%f, \"y0\":%f, \"x1\":%f, \"y1\":%f}", pair.x0,
              pair.y0, pair.x1, pair.y1);
      if (a != total_number_of_pairs - 1) {
        fprintf(data, ",\n");
      } else {
        fprintf(data, "\n");
      }
    }
    fprintf(data, "]}");
    fclose(data);
  }

  {
    // write haversine distances as binary, last number is the average.
    FILE *binary_distances = fopen("binary_distances.f64", "wb");
    if (binary_distances == NULL) {
      exit_code = EXIT_FAILURE;
      fprintf(stderr, "failed to open the file");
      goto exit;
    }
    int n = fwrite(distances, sizeof(f64) * total_number_of_pairs, 1,
                   binary_distances);
    if (n != 1) {
      exit_code = EXIT_FAILURE;
      fprintf(stderr, "failed to write haversine distances to binary file\n");
    }
    n = fwrite(&average_distance, sizeof(f64), 1, binary_distances);
    if (n != 1) {
      exit_code = EXIT_FAILURE;
      fprintf(
          stderr,
          "failed to write average distance to the end of the binary file\n");
    }

    fclose(binary_distances);
  }
exit:
  free(distances);
  free(point_pairs);
  return exit_code;
}