#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

GLuint load_shader(const char *shader_path, GLenum shader_type) {
	struct stat s;
        if (stat(shader_path, &s) < 0) {
		perror("failed to stat shader file");
		return 0;
	}

	char *shader_source = malloc(s.st_size);
	FILE *file = fopen(shader_path, "r");
	int read = fread(shader_source, s.st_size, 1, file);
	if (read != 1) {
		free(shader_source);
		fclose(file);
		perror("failed to read shader file");
		return 0;
	}
	fclose(file);

	GLuint shader = glCreateShader(shader_type);
        glShaderSource(shader, 1, (char const * const *)&shader_source, (const GLint *)&(s.st_size));
        glCompileShader(shader);
        free(shader_source);
	int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(!success){
                char infoLog[512];
                glGetShaderInfoLog(shader, 512, NULL, infoLog);
                fprintf(stderr, "failed to compile vertex shader: %s\n", infoLog);
                return 0;
        }
	return shader;
}
uint32_t create_program(const char *vertex_shader_path, char *fragment_shader_path) {

	GLuint vertex_shader = load_shader(vertex_shader_path, GL_VERTEX_SHADER);
	GLuint fragment_shader = load_shader(fragment_shader_path, GL_FRAGMENT_SHADER);

	uint32_t shader_program = glCreateProgram();
        glAttachShader(shader_program, vertex_shader);
        glAttachShader(shader_program, fragment_shader);
        glLinkProgram(shader_program);
	int success;
        glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
        if(!success) {
                char infoLog[512];
                glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
                fprintf(stderr, "failed to link program: %s\n", infoLog);
                return 0;
        }

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

	return shader_program;
}
