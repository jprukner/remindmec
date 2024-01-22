#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void selection_sort_asm(int64_t *numbers, uint64_t length);

void selectsort(int64_t *numbers, uint64_t length) {
	for(uint64_t n = 0; n < length - 1; n++) {
		uint64_t min_index = n;
		for(uint64_t m = n+1; m < length; m++) {
			if(numbers[m] < numbers[min_index]) {
				min_index = m;
			}
		}
		if (min_index != n) {
			uint64_t tmp = numbers[n];
			numbers[n] = numbers[min_index];
			numbers[min_index] = tmp;
		}
	}
}

int main(uint64_t argc, char *argv[]) {
	uint64_t length = argc -1;
	int64_t * numbers = malloc(sizeof(int64_t)*length);
	if(numbers == NULL) {
		fprintf(stderr, "failed to allocate memory for numbers\n");
		return EXIT_FAILURE;
	}
	for(int n = argc -1 ; n > 0; n--){
		printf("%d. argument is %s\n", n, argv[n]);
		numbers[n-1] = atoll(argv[n]);
	}
	selection_sort_asm(numbers, length);
	for(int n = 0; n < length; n++) {
		printf("%lld\n", numbers[n]);
	}
	free(numbers);
	return EXIT_SUCCESS;

}
