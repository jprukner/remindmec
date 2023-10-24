#ifndef SHADER_LOADER_H
#define SHADER_LOADER_H 1

#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

GLuint load_shader(const char *shader_path, GLenum shader_type);
GLuint create_program(const char *vertex_shader_path,
                      char *fragment_shader_path);
#endif
