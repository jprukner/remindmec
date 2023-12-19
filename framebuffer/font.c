#include <ft2build.h>
#include FT_FREETYPE_H

#include "shorttypes.h"
#include "api.h"
#include "font.h"

struct font font_init(u16 height, const u8 *font_file_path, enum font_init_error *error) {
	struct font font;
	*error = FONT_INIT_ERROR_NO_ERROR;
        if (FT_Init_FreeType (&font.ft) != 0){
                perror("failed to init freetype library");
		*error = FONT_INIT_ERROR_FREETYPE_LIBRARY_INIT_FAILED;
		goto cleanup;
        }

        if (FT_New_Face (font.ft, "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf", 0, &font.face) != 0){
                perror("failed to load font to init face");
		*error = FONT_INIT_ERROR_FONT_LOADING_FAILED;
		goto cleanup;
        }

        if (FT_Set_Pixel_Sizes (font.face, 0, height) != 0) {
                perror("failed to set height");
		*error = FONT_INIT_ERROR_FONT_SIZE_SETTING_FAILED;
		goto cleanup;
        }
	return font;
cleanup:
        FT_Done_FreeType (font.ft);
	font = (const struct font){0};
	return font;
}

void font_free(struct font font) {
	 FT_Done_FreeType (font.ft);
}

u16 font_put_character(struct font font, u16 x, u16 y, u32 character, struct window_properties properties, u8 *buffer) {
        FT_UInt gi = FT_Get_Char_Index (font.face, character);
        if (gi == 0) {
                perror("failed to get character index");
                return 0;
        }
        FT_Load_Glyph (font.face, gi, FT_LOAD_DEFAULT);
        u16 bbox_ymax = font.face->bbox.yMax / 64;
        u16 glyph_width = font.face->glyph->metrics.width / 64;
        u16 advance = font.face->glyph->metrics.horiAdvance / 64;
        u16 x_off = (advance - glyph_width) / 2;
        u16 y_off = bbox_ymax - font.face->glyph->metrics.horiBearingY / 64;
        FT_Render_Glyph(font.face->glyph, FT_RENDER_MODE_NORMAL);

	for (u16 i = 0; i < (u16)font.face->glyph->bitmap.rows; i++){
		u32 row_offset = y + i + y_off;
		for (u16 j = 0; j < (u16)font.face->glyph->bitmap.width; j++){
			u8 p = font.face->glyph->bitmap.buffer [i * font.face->glyph->bitmap.pitch + j];
			if (p) {
				u32 offset = ((row_offset*properties.width)+x+j+x_off)*properties.bytes_per_pixel;
				buffer[offset + 0] = p;
				buffer[offset + 1] = p;
				buffer[offset + 2] = p;
			}
		}
	}
	return advance;
}

