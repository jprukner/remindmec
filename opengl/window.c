#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "shader.h"
#include "texture.h"
#include "matrix.h"

#define SCR_WIDTH 1200
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
      -0.5f, 0.5f,  0.0f, 0.0f, 1.0f, // top left, front
      0.5f,  0.5f,  0.0f, 1.0f, 1.0f, // top right, front
      -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, // bottom left, front
      0.5f,  -0.5f, 0.0f, 1.0f, 0.0f // bottom right, front

      -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, // top left, back
      0.5f,  0.5f, -0.5f, 1.0f, 1.0f, // top right, back
      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, // bottom left, back
      0.5f,  -0.5f, -0.5f, 1.0f, 0.0f // bottom right, back
  };

  unsigned int indecies[] = {
	// front
	2, 3, 0,
	0, 1, 3,
	// bottom
	2, 6, 3,
	3, 7, 6,
	// top
	0, 4, 5,
	5, 1, 0,
	// left
	0, 2, 6,
	6, 4, 0,
	// right
	3, 1, 5,
	5, 7, 3,
	// back
	6, 4, 5,
	5, 7, 6
  };

  glEnable(GL_DEPTH_TEST);


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

  GLuint rotation_location = glGetUniformLocation(shaderProgram, "rotation");
  if(rotation_location < 0) {
	glfwTerminate();
	return 1;
  }

  GLuint translation_location = glGetUniformLocation(shaderProgram, "translation");
  if(translation_location < 0) {
	glfwTerminate();
	return 1;
  }

  GLuint projection_location = glGetUniformLocation(shaderProgram, "projection");
  if(projection_location < 0) {
	glfwTerminate();
	return 1;
  }


  float rotation[16];
  float translation[16] = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 2.0f,
	0.0f, 0.0f, 0.0f, 1.0f
  };
  float projection[16];
  projection_matrix(M_PI_2, (float)SCR_WIDTH/(float)SCR_HEIGHT, 0.5f, 10.0f, projection);
//  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  while (!glfwWindowShouldClose(window)) {
    processInput(window);
    // ------
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);
    // -- draw a triangle
    glBindTexture(GL_TEXTURE_2D, texture);
    glUseProgram(shaderProgram);
    rotation_matrix_y(rotation, (float)glfwGetTime());
    glUniformMatrix4fv(rotation_location, 1, GL_TRUE, rotation);
    glUniformMatrix4fv(translation_location, 1, GL_TRUE, translation);
    glUniformMatrix4fv(projection_location, 1, GL_TRUE, projection);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    // ----
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwTerminate();
  return 0;
}
