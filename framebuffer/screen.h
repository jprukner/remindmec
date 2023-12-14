#ifndef SCREEN_H
#define SCREEN_H 1

#include "shorttypes.h"

enum screen_init_error {
	SCREEN_INIT_NO_ERROR,
	SCREEN_INIT_OPEN_FRAME_BUFFER_ERROR,
	SCREEN_INIT_IOCTL_FBIOGET_VSCREENINFO_ERROR,
	SCREEN_INIT_FRAMEBUFFER_MMAP_ERROR
};

struct screen {
	u8 * buffer;
	u8 * temporary_buffer;
	u16 width;
	u16 height;
	u16 bytes_per_pixel;
};


void screen_perror(enum screen_init_error error);
struct screen screen_init(enum screen_init_error *error);
void screen_free(struct screen screen);
void screen_place_window(struct screen screen, u16 top_left_x, u16 top_left_y, u16 width, u16 height, u8 *window);
void screen_swap_buffers(struct screen screen);

#endif
