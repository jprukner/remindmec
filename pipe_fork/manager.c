#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>



int main(int argc, char*argv[]){
	char * hello = "hello";
	int memFd = shm_open("example_memory", O_CREAT | O_RDWR, S_IRWXU);
	if (memFd == -1)
	{
	    perror("Can't open file");
	    return 1;
	}

	int res = ftruncate(memFd, sizeof(hello));
	if (res == -1)
	{
	    perror("Can't truncate file");
	    return res;
	}
	char *buffer = mmap(NULL, sizeof(hello), PROT_READ | PROT_WRITE, MAP_SHARED, memFd, 0);
	if (buffer == NULL)
	{
	    perror("Can't mmap");
	    return -1;
	}

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
		size_t sizeof_hello = sizeof(hello);
            	write(fd[1], &sizeof_hello, sizeof(size_t));
	        close(fd[1]);
		int status = 0;
		while(wait(&status) > 0){
			printf("child terminated: %d\n", status);
		}
		close(memFd);
		shm_unlink("example_memory");
	}

}
