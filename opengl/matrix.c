#include <stdio.h>
#include <string.h>
#include <math.h>

void print_matrix(float* matrix, unsigned int width, unsigned int height){
	for(int h = 0; h < height;h++) {
		for(int w = 0; w < width; w++) {
			printf("%f, ", matrix[(h*width)+w]);
		}
		printf("\n");
	}
}

void transpose(float* matrix, unsigned width, unsigned int height){
	float output[width*height];
	unsigned int counter = 0;
	for(int w = 0; w < width; w++) {
		for(int h = 0; h < height;h++) {
			output[counter] = matrix[(h*width)+w];
			counter++;
		}
	}
	memcpy(matrix, output, width*height);
}

// rotation_matrix_x expects 4x4 matrix;
void rotation_matrix_z(float *matrix, float angle){
	float rotation[] = {
		cos(angle), -sin(angle), 0, 0,
		sin(angle), cos(angle),  0, 0,
		0,          0,           1, 0,
		0,          0,           0, 1
	};
	memcpy(matrix, rotation, sizeof(rotation));
}
