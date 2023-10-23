#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdbool.h>
#define SCR_WIDTH 800
#define SCR_HEIGHT 600

void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main() {

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		fprintf(stderr, "failed to create a window");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		perror("failed to init glad:");
		return 1;
	}

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f, 0.5f, 0.0f
	};

	uint32_t VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	const char *vertexShaderSource = "#version 330 core\n"
		"layout (location = 0) in vec3 aPos;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
		"}\0";

	uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	int success;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if(!success){
		char infoLog[512];
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		fprintf(stderr, "failed to compile vertex shader: %s\n", infoLog);
		return 1;
	}

	const char *fragmentShaderSource = "#version 330 core\n"
		"out vec4 color;\n"
		"void main()\n"
		"{\n"
		"  color = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n";
		"}\0";
	uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	if(!success){
                char infoLog[512];
                glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
                fprintf(stderr, "failed to compile fragment shader: %s\n", infoLog);
                return 1;
        }

	// todo shader program
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

   while (!glfwWindowShouldClose(window))
    {
	processInput(window);
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
	// ----
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
	   glfwTerminate();
	return 0;
}
