#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "shader.h"
#include "texture.h"
#include "matrix.h"

#define SCR_WIDTH 800
#define SCR_HEIGHT 600

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

int main() {

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
  if (window == NULL) {
    fprintf(stderr, "failed to create a window");
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    perror("failed to init glad:");
    return 1;
  }

  float vertices[] = {
      // coordinates      // texture coordinates
      -0.5f, 0.5f,  0.0f, 0.0f, 1.0f, // top left
      0.5f,  0.5f,  0.0f, 1.0f, 1.0f, // top right
      -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, // bottom left
      0.5f,  -0.5f, 0.0f, 1.0f, 0.0f // bottom right
  };

  unsigned int indecies[] = {0, 1, 2, 1, 2, 3};

  uint32_t VBO, VAO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indecies), indecies,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3*sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  GLuint shaderProgram = create_program("../vertex.vs", "../fragment.fs");
  if (shaderProgram == GL_INVALID_VALUE) {
	glfwTerminate();
	return 1;
  }

  GLuint texture = load_image_as_texture("../bricks.png");

  if (texture == GL_INVALID_VALUE) {
	glfwTerminate();
	return 1;
  }

  GLuint transform_location = glGetUniformLocation(shaderProgram, "transform");
  if(transform_location < 0) {
	glfwTerminate();
	return 1;
  }
  float transform[16];

 // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  while (!glfwWindowShouldClose(window)) {
    processInput(window);
    // ------
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    // -- draw a triangle
    glBindTexture(GL_TEXTURE_2D, texture);
    glUseProgram(shaderProgram);
    rotation_matrix_z(transform, (float)glfwGetTime());
    glUniformMatrix4fv(transform_location, 1, GL_TRUE, transform);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    // ----
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwTerminate();
  return 0;
}
