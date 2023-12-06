#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdio.h>
#include <unistd.h>

int main(int argc, char*argv[]){
	size_t shared_memory_size;
	ssize_t bytes_read = read(STDIN_FILENO, &shared_memory_size, sizeof(size_t));
	if (bytes_read != sizeof(size_t)){
		fprintf(stderr, "expected to size of shared memory to be size_t of size %lu but %lu bytes were available in stdin\n", sizeof(size_t), bytes_read);
		return 1;
	}
	if (shared_memory_size == 0) {
		fprintf(stderr, "expected non-zero value for size of shared memory at stdin\n");
		return 1;
	}
	int memFd = shm_open("example_memory", O_CREAT | O_RDWR, S_IRWXU);
	if (memFd == -1)
	{
	    perror("Can't open file");
	    return 1;
	}

	int res = ftruncate(memFd, shared_memory_size);
	if (res == -1)
	{
	    perror("Can't truncate file");
	    return res;
	}
	char *buffer = mmap(NULL, shared_memory_size, PROT_READ | PROT_WRITE, MAP_SHARED, memFd, 0);
	if (buffer == NULL)
	{
	    perror("Can't mmap");
	    return -1;
	}

	printf("user window: %s\n", buffer);
}
