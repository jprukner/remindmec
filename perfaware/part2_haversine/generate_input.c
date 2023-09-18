#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define CLUSTER_COUNT 2
#define NUMBER_OF_POINTS_IN_CLUSTER 20
#define CLUSTER_RANGE 60

int16_t random_in_range(int16_t lower, int16_t upper) {
  return (rand() % (upper - lower + 1)) + lower;
}

int main(int argc, char *argv[]) {
  for (int n = 0; n < CLUSTER_COUNT; ++n) {
    uint64_t points_count = 0;
    int16_t cluster_centre_x = random_in_range(-180, 180);
    int16_t cluster_centre_y = random_in_range(-90, 90);
    printf("----cluster %d with centre [\"x\":%d, \"y\"%d]-----\n", n,
           cluster_centre_x, cluster_centre_y);
    while (points_count < NUMBER_OF_POINTS_IN_CLUSTER) {
      int16_t new_point_x_start = cluster_centre_x - (CLUSTER_RANGE / 2);
      if (new_point_x_start < -180) {
        new_point_x_start = (new_point_x_start + 180) + 180;
      }
      int16_t new_point_x_stop = cluster_centre_x + (CLUSTER_RANGE / 2);
      if (new_point_x_stop > 180) {
        new_point_x_stop = (new_point_x_stop - 180) - 180;
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

      points_count += 1;
      printf("[\"x\":%d, \"y\":%d]\n", new_point_x, new_point_y);
    }
  }
}