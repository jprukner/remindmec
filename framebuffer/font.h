#ifndef FONT_H
#define FONT_H 1

#include <ft2build.h>
#include FT_FREETYPE_H

#include "shorttypes.h"
#include "api.h"

struct font {
	FT_Library ft;
	FT_Face face;
};

enum font_init_error {
		FONT_INIT_ERROR_NO_ERROR,
		FONT_INIT_ERROR_FREETYPE_LIBRARY_INIT_FAILED,
		FONT_INIT_ERROR_FONT_LOADING_FAILED,
		FONT_INIT_ERROR_FONT_SIZE_SETTING_FAILED
};

struct font font_init(u16 height, const u8 *font_file_path, enum font_init_error *error);
void font_free(struct font font);
u16 font_put_character(struct font font, u16 x, u16 y, u32 character, struct window_properties properties, u8 *buffer);
#endif
