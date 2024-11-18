#ifndef FONT_H_
#define FONT_H_

/* RLE scheme:
 * 00xxxxxx: single pixel, value in low 6 bits
 * 01xxxxxx: repeat last pixel, count in low 6 bits
 * 1xxxxxxx: skip pixels, count in low 7 bits
 */

struct glyph {
	int x, y, width, height;
	int orig_x, orig_y;
	int advance;

	unsigned char *rle;
};

struct font {
	unsigned int xsz, ysz;
	unsigned int xshift;
	unsigned char *pixels;
	int line_advance;
	int baseline;
	struct glyph glyphs[256];
};

struct fontdraw {
	unsigned char *img, *ptr;
	int curx, cury, width, height;
	int pitch;
	int colorshift;
	unsigned char colorbase;
};

struct font *fnt_load(const char *fname);
void fnt_free(struct font *fnt);

void fnt_position(struct fontdraw *draw, int x, int y);
void fnt_drawglyph(struct font *fnt, struct fontdraw *draw, int c);

void fnt_drawstr(struct font *fnt, struct fontdraw *draw, const char *str);


#endif	/* FONT_H_ */
