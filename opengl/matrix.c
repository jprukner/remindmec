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

void rotation_matrix_y(float *matrix, float angle){
	float rotation[] = {
		cos(angle), 0, sin(angle), 0,
		0,          1, 0,          0,
		-sin(angle),0, cos(angle), 0,
		0,          0,           0, 1
	};
	memcpy(matrix, rotation, sizeof(rotation));
}


void projection_matrix(float fov, float width_to_height_ratio, float near_z, float far_z, float *matrix) {
	float z_range = near_z - far_z;
	float a = (-far_z - near_z)/z_range;
	float b = (2.0f * far_z * near_z)/z_range;
	float p = 1/tan(fov/2.0f);
	float pp = 1/(tan(fov/2.0f)*width_to_height_ratio);
	float projection[16] = {
		pp,   0.0f, 0.0f, 0.0f,
		0.0f, p,    0.0f, 0.0f,
		0.0f, 0.0f, a,    b,
		0.0f, 0.0f, 1.0f, 0.0f
	};
//	float projection[] = {
//		p, 0, 0, 0,
//		0, p, 0, 0,
//		0, 0, 1, 0,
//		0, 0, 1, 0
//	};
	print_matrix(projection, 4, 4);
	memcpy(matrix, projection, sizeof(projection));
}
