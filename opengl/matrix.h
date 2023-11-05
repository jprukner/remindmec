#ifndef MATRIX_H
#define MATRIX_H 1

void transpose(float* matrix, unsigned int width, unsigned int height);
void print_matrix(float* matrix, unsigned int width, unsigned int height);
void rotation_matrix_z(float *matrix, float angle);
void rotation_matrix_y(float *matrix, float angle);
void projection_matrix(float fov, float width_to_height_ratio, float near_z, float far_z, float *matrix);

#endif
