
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>

#include "shorttypes.h"
#include "screen.h"

static char *errors_mapping[] = {
	"failed to open framebuffer",
	"failed to get screen info using ioctl",
	"failed to map framebuffer to memory using mmap"
};

void screen_perror(enum screen_init_error error) {
	if (error == SCREEN_INIT_NO_ERROR) {
		return;
	}
	perror(errors_mapping[error-1]);
}

struct screen screen_init(enum screen_init_error *error) {
	struct screen screen = {0};
	s32 fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd < 0) {
		*error = SCREEN_INIT_OPEN_FRAME_BUFFER_ERROR;
		return screen;
	}

	struct fb_var_screeninfo vinfo;

	if (ioctl (fbfd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
		*error = SCREEN_INIT_IOCTL_FBIOGET_VSCREENINFO_ERROR;
		goto cleanup;
	}

	u64 size = vinfo.xres_virtual * vinfo.yres_virtual * (vinfo.bits_per_pixel/8);
	u8 *buffer = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, (off_t)0);
	if (buffer < 0) {
		*error = SCREEN_INIT_FRAMEBUFFER_MMAP_ERROR;
		goto cleanup;
	}
	screen.temporary_buffer = malloc(size);
	memset(screen.temporary_buffer, 0, size);
	screen.buffer = buffer;
	screen.width = vinfo.xres_virtual;
	screen.height = vinfo.yres_virtual;
	screen.bytes_per_pixel = vinfo.bits_per_pixel / 8;
cleanup:
	close(fbfd);
	return screen;
}

void screen_free(struct screen screen){
	uint64_t size = screen.width * screen.height * screen.bytes_per_pixel;
	munmap(screen.buffer, size);
	free(screen.temporary_buffer);
}

void screen_place_window(struct screen screen, u16 top_left_x, u16 top_left_y, u16 width, u16 height, u8 *window) {
	for(u16 y=0; y < height; y++) {
		u32 destination_offset = ((y+top_left_y)*screen.width + top_left_x)*screen.bytes_per_pixel;
		u32 source_offset = (y*width)*screen.bytes_per_pixel;
		memcpy((void*)(screen.temporary_buffer+destination_offset), (void*)(window+source_offset), sizeof(u8)*width*screen.bytes_per_pixel);
	}
}

void screen_swap_buffers(struct screen screen) {
	u64 size = screen.width * screen.height * screen.bytes_per_pixel;
	memcpy(screen.buffer, screen.temporary_buffer, size);
	memset(screen.temporary_buffer, 0, size);
}
