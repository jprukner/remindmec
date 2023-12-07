
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
	int fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd < 0) {
		perror("failed to open framebuffer");
		return 1;
	}

	struct fb_var_screeninfo vinfo;

	ioctl (fbfd, FBIOGET_VSCREENINFO, &vinfo);

	int fb_width = vinfo.xres_virtual;
	int fb_height = vinfo.yres_virtual;
	int fb_bpp = vinfo.bits_per_pixel;
	int fb_bytes = fb_bpp / 8;

	printf("%dx%d pixels at %d bytes per pixel\n", fb_width, fb_height, fb_bytes);


	unsigned char *window = malloc(4*sizeof(unsigned char)*200*200);
	{
		unsigned char r=255;
		unsigned char g=255;
		unsigned char b=0;
		for(int y=0;y<200;y++){
			for(int x=0;x<200;x++){
				int offset = (y* 200 + x)*4;
				window[offset + 0] = r;
				window[offset + 1] = g;
				window[offset + 2] = b;
				window[offset + 3] = 0;
			}
		}
	}

	int fb_data_size = fb_width * fb_height * fb_bytes;
	unsigned char *screen = mmap (0, fb_data_size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, (off_t)0);
	{
		for(int y=0;y<fb_height;y++){
			for(int x=0;x<fb_width;x++){
				int offset = (y*fb_width + x)*4;
				screen[offset + 0] = y%256;
				screen[offset + 1] = x%256;
				screen[offset + 2] = 0;
				screen[offset + 3] = 0;

			}
		}
	}
	int target_x = 0;
	int target_y = 0;
	unsigned char *buffer = malloc(fb_data_size);
	while(1){
		memset (buffer, 0, fb_data_size);
		if (target_y + 200 > fb_height) {
			target_y = 0;
		}
		if (target_x + 200 > fb_width) {
			target_x = 0;
		}
		for(int y=0; y < 200; y++) {
			int destination_offset = ((y+target_y)*fb_width + target_x)*4;
			int source_offset = (y*200)*4;
			memcpy((void*)(buffer+destination_offset), (void*)(window+source_offset), sizeof(unsigned char)*200*4);
		}
		target_y++;
		target_x++;
		memcpy(screen, buffer, fb_data_size);
		usleep(16000);
	}
	munmap(screen, fb_data_size);
	close(fbfd);
}
