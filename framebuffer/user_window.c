#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>

#include "api.h"
#include "shorttypes.h"
#include "font.h"

static sem_t stop;
static void sig_handler(int signal_number) {
        printf("user_window: got signal %d\n", signal_number);
        sem_post(&stop);
}

int main(int argc, char*argv[]){
        sem_init(&stop, 0, 0);
        signal(SIGINT, sig_handler);
        signal(SIGTERM, sig_handler);

	int return_code = 0;

	struct window_properties properties;
	size_t size_of_window_properties = sizeof(struct window_properties);
	ssize_t bytes_read = read(STDIN_FILENO, &properties, size_of_window_properties);
	printf("window_id: %s, width: %d\n", properties.window_id, properties.width);
	if (bytes_read != size_of_window_properties){
		printf("expected windows properties of size %lu but %lu bytes were available in stdin\n", sizeof(size_t), bytes_read);
		return 1;
	}
	size_t shared_memory_size = properties.width * properties.height * properties.bytes_per_pixel;
	if (shared_memory_size == 0) {
		printf("expected non-zero value for size of window buffer, got 0 at stdin inside passed window properties\n");
		return 1;
	}
	int memFd = shm_open(properties.window_id, O_CREAT | O_RDWR, S_IRWXU);
	if (memFd == -1)
	{
	    perror("Can't open file");
	    return 1;
	}

	int res = ftruncate(memFd, shared_memory_size);
	if (res == -1)
	{
		perror("Can't truncate file");
		return_code = res;
		goto cleanup_1;
	}
	char *buffer = mmap(NULL, shared_memory_size, PROT_READ | PROT_WRITE, MAP_SHARED, memFd, 0);
	if (buffer == NULL)
	{
		perror("Can't mmap");
		return_code = 1;
		goto cleanup_1;
	}

	sem_t *semaphore = sem_open(properties.window_id, 0);
	if(semaphore == SEM_FAILED) {
		perror("failed to open semaphore");
		return_code = 1;
		goto cleanup_2;
	}

	enum font_init_error error;
	struct font font = font_init(properties.height/10, "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf", &error);
	if (error != FONT_INIT_ERROR_NO_ERROR) {
		goto cleanup_3;
	}

	// update loop
	int n=0;
	int should_stop = 0;
	u16 x = 0;
	u16 y = 0;
	while(n < 10 && !should_stop){
		// setup glyph
		u32 character = (n%(255-32))+' ';
		// --
		sem_wait(semaphore);
		if (x > properties.width) {
			x = 0;
			memset(buffer, 0, properties.width*properties.height*properties.bytes_per_pixel);
		}
		u16 advance = font_put_character(font, x, y, character, properties, buffer);
		sem_post(semaphore);
		// Move the x position, ready for the next character.
		x += advance;
		n++;
		usleep(320000);
		if (sem_getvalue(&stop, &should_stop) < 0) {
			perror("sem_getvalue()");
			break;
		}
	}
	// ---

cleanup_4:
	font_free(font);
cleanup_3:
        sem_close(semaphore);
cleanup_2:
        munmap(buffer, shared_memory_size);
cleanup_1:
        shm_unlink(properties.window_id);
        close(memFd);
	sem_destroy(&stop);
        return return_code;
}
