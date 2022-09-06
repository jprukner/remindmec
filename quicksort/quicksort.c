#include <stddef.h>
#include <stdio.h>

void print_array(int array[], size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (i != length - 1) {
      printf("%d,", array[i]);
    } else {
      printf("%d\n", array[i]);
    }
  }
}

void quicksort(int array[], int start, int stop) {
  int length = stop - start + 1;
  if (length <= 1) {
    return;
  }
  int pivot = array[stop];
  int i = start - 1;
  int j = start;
  while (j < stop) {
    if (array[j] < pivot) {
      i++;
      int tmp = array[i];
      array[i] = array[j];
      array[j] = tmp;
    }
    j++;
  }
  int tmp = array[i + 1];
  array[i + 1] = pivot;
  array[stop] = tmp;
  quicksort(array, start, i);
  quicksort(array, i + 2, stop);
}

int main() {
  int array[] = {7, 2, 1, 8, 6, 3, 5, 4};

  print_array(array, 8);
  quicksort(array, 0, 7);
  print_array(array, 8);
  return 0;
}
