#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <signal.h>

#include "api.h"
#include "screen.h"

static sem_t stop;
static void sig_handler(int signal_number) {
	printf("manager: got signal %d\n", signal_number);
	sem_post(&stop);
}


int main(int argc, char*argv[]){
	sem_init(&stop, 0, 0);
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	int return_code = 0;

	struct window_properties properties = {
		.window_id = "/12345678912345",
		.width = 200,
		.height = 200,
		.bytes_per_pixel = 4
	};
	size_t size_of_window_buffer = properties.width * properties.height * properties.bytes_per_pixel;
	int memFd = shm_open(properties.window_id, O_CREAT | O_RDWR, S_IRWXU);
	if (memFd == -1)
	{
	    perror("failed to open shared memory file");
	    return 1;
	}

	int res = ftruncate(memFd, size_of_window_buffer);
	if (res == -1)
	{
		perror("failed truncate shared memory");
		return_code = res;
		goto cleanup_1;
	}
	unsigned char *buffer = mmap(NULL, size_of_window_buffer, PROT_READ | PROT_WRITE, MAP_SHARED, memFd, 0);
	if (buffer == NULL)
	{
		perror("failed to mmap shared memory");
		return_code = 1;
		goto cleanup_1;
	}
	memset(buffer, 0, size_of_window_buffer);

	sem_t *semaphore = sem_open(properties.window_id, O_CREAT | O_EXCL, S_IRWXU, 1);
	if(semaphore == SEM_FAILED) {
		perror("failed to create semaphore");
		return_code = 1;
		goto cleanup_2;
	}
	printf("successfully created named semaphore '%s'\n", properties.window_id);

	enum screen_init_error error;
        struct screen screen = screen_init(&error);
        if(error != SCREEN_INIT_NO_ERROR) {
                screen_perror(error);
                goto cleanup_3;
        }

	int fd[2] = {0};
	if(pipe(fd)) {
		perror("failed to create pipe");
		return_code = 1;
		goto cleanup_3;
	}
	pid_t pid = fork();
	if(pid < 0) {
		perror("failed to fork");
		return_code = 1;
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

	        // update loop
	        int n=0;
		int should_stop = 0;
	        while(n < 5000 && !should_stop){
	                sem_wait(semaphore);
	                        // copy to temp buffer
				screen_place_window(screen, 0, 0, properties.width, properties.height, buffer);
	                sem_post(semaphore);
			n++;
			usleep(32000);
			if (sem_getvalue(&stop, &should_stop) < 0) {
				perror("sem_getvalue()");
				break;
			}
			screen_swap_buffers(screen);
	        }
	        // ---

		int status = 0;
		while(wait(&status) > 0) {};
		printf("done\n");
	}

	screen_free(screen);
cleanup_3:
	sem_destroy(&stop);
	sem_close(semaphore);
	sem_unlink(properties.window_id);
cleanup_2:
	munmap(buffer, size_of_window_buffer);
cleanup_1:
	shm_unlink(properties.window_id);
	close(memFd);

	return return_code;
}
