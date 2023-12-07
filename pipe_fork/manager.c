#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#include "api.h"

int main(int argc, char*argv[]){
	struct window_properties properties = {
		.window_id = "123456789123456",
		.width = 200,
		.height = 200,
		.bytes_per_pixel = 4
	};
	size_t size_of_window_buffer = properties.width * properties.height * properties.bytes_per_pixel;
	int memFd = shm_open(properties.window_id, O_CREAT | O_RDWR, S_IRWXU);
	if (memFd == -1)
	{
	    perror("Can't open file");
	    return 1;
	}

	int res = ftruncate(memFd, size_of_window_buffer);
	if (res == -1)
	{
	    perror("Can't truncate file");
	    return res;
	}
	unsigned char *buffer = mmap(NULL, size_of_window_buffer, PROT_READ | PROT_WRITE, MAP_SHARED, memFd, 0);
	if (buffer == NULL)
	{
	    perror("Can't mmap");
	    return -1;
	}

	memset(buffer, 0, size_of_window_buffer);
	char hello[] = "hello world";
	memcpy(buffer, hello, sizeof(hello));
	int fd[2] = {0};
	if(pipe(fd)) {
		perror("failed to create pipe");
		return 1;
	}
	pid_t pid = fork();
	if(pid < 0) {
		perror("failed to fork");
	} else if(pid == 0) {
		printf("I am a child: %d\n", pid);
		close(fd[1]);
		dup2(fd[0], STDIN_FILENO);
		close(fd[0]);
		execl("./user_window", NULL);
	} else {
		printf("I am a parent: %d\n", pid);
		close(fd[0]);  // Close read end
		write(fd[1], &properties, sizeof(struct window_properties));
	        close(fd[1]);
		int status = 0;
		while(wait(&status) > 0){
			printf("child terminated: %d\n", status);
		}
		munmap(buffer, size_of_window_buffer);
		close(memFd);
		shm_unlink(properties.window_id);
	}
}
