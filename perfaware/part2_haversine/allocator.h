#ifndef HUGE_ALLOCATOR_H
#define HUGE_ALLOCATOR_H
#include <sys/mman.h>
#include <string.h>
#define HUGE_TABLE_2MB_SIZE (2*1024*1024)

uint64_t required_pages(size_t size) {
	uint64_t pages_required = size / HUGE_TABLE_2MB_SIZE;
        if (size % HUGE_TABLE_2MB_SIZE != 0) {
                pages_required +=1;
        }
	return pages_required;
}

void *huge_malloc(size_t size){
	return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
}

int huge_free(void*addr, size_t size){
	return munmap(addr, required_pages(size)*HUGE_TABLE_2MB_SIZE);
}

void *huge_realloc(void* addr, size_t old_size, size_t new_size) {
	if (required_pages(old_size) == required_pages(new_size)) {
		return addr;
	}
	void *new_addr = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
	if(new_addr == MAP_FAILED) {
		return MAP_FAILED;
	}
	memcpy(new_addr, addr, old_size);
	int status = munmap(addr, required_pages(old_size)*HUGE_TABLE_2MB_SIZE);
	if(status < 0) {
		return MAP_FAILED;
	}
	return new_addr;
}
#endif

