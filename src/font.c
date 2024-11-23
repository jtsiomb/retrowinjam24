#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include "font.h"
#include "util.h"


struct font *fnt_load(const char *fname)
{
	FILE *fp;
	char buf[512];
	int hdr_lines = 0;
	struct font *fnt;
	struct glyph *g;
	int i, max_pixval = 255, num_pixels, max_height = 0;
	int greyscale = 0;

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "failed to open font: %s: %s\n", fname, strerror(errno));
		return 0;
	}

	if(!(fnt = calloc(1, sizeof *fnt))) {
		perror("failed to allocate font");
		return 0;
	}
	fnt->line_advance = INT_MIN;

	while(hdr_lines < 3) {
		char *line = buf;
		if(!fgets(buf, sizeof buf, fp)) {
			perror("unexpected end of file");
			goto err;
		}

		while(isspace(*line)) {
			line++;
		}

		if(line[0] == '#') {
			int c, res;
			int x, y, xsz, ysz, orig_x, orig_y, adv, line_adv, baseline;
			int ptsize;

			if((res = sscanf(line + 1, " size: %d\n", &ptsize)) == 1) {
				/* ignore size */

			} else if((res = sscanf(line + 1, " advance: %d\n", &line_adv)) == 1) {
				fnt->line_advance = line_adv;

			} else if((res = sscanf(line + 1, " baseline: %d\n", &baseline)) == 1) {
				fnt->baseline = baseline;

			} else if((res = sscanf(line + 1, " %d: %dx%d+%d+%d o:%d,%d adv:%d\n",
							&c, &xsz, &ysz, &x, &y, &orig_x, &orig_y, &adv)) == 8) {
				if(c >= 0 && c < 256) {
					g = fnt->glyphs + c;
					g->x = x;
					g->y = y;
					g->width = xsz;
					g->height = ysz;
					g->orig_x = orig_x;
					g->orig_y = orig_y;
					g->advance = adv;
					g->imgptr = (unsigned char*)1;	/* mark it used for the precalc below */

					if(ysz > max_height) max_height = ysz;
				}

			} else {
				fprintf(stderr, "%s: invalid glyph info line\n", __func__);
				goto err;
			}

		} else {
			switch(hdr_lines) {
			case 0:
				if(line[0] != 'P' || !(line[1] == '6' || line[1] == '5')) {
					fprintf(stderr, "%s: invalid file format (magic)\n", __func__);
					goto err;
				}
				greyscale = line[1] == '5';
				break;

			case 1:
				if(sscanf(line, "%u %u", &fnt->xsz, &fnt->ysz) != 2) {
					fprintf(stderr, "%s: invalid file format (dim)\n", __func__);
					goto err;
				}
				break;

			case 2:
				{
					char *endp;
					max_pixval = strtol(line, &endp, 10);
					if(endp == line) {
						fprintf(stderr, "%s: invalid file format (maxval)\n", __func__);
						goto err;
					}
				}
				break;

			default:
				break;	/* can't happen */
			}
			hdr_lines++;
		}
	}

	num_pixels = fnt->xsz * fnt->ysz;
	if(!(fnt->pixels = malloc(num_pixels))) {
		perror("failed to allocate pixels");
		goto err;
	}

	for(i=0; i<num_pixels; i++) {
		int c = fgetc(fp);
		if(c == -1) {
			fprintf(stderr, "unexpected end of file while reading pixels\n");
			goto err;
		}
		fnt->pixels[i] = 255 * c / max_pixval;
		if(!greyscale) {
			fgetc(fp);
			fgetc(fp);
		}
	}

	fnt->xshift = calc_shift(fnt->xsz);

	g = fnt->glyphs;
	for(i=0; i<256; i++) {
		if(g->imgptr) {
			int row = g->y / max_height;
			g->y = row * max_height;
			g->height = max_height;
			g->imgptr = fnt->pixels + (g->y << fnt->xshift) + g->x;
		}
		g++;
	}
	return fnt;

err:
	fnt_free(fnt);
	return 0;
}

void fnt_free(struct font *fnt)
{
	if(!fnt) return;
	free(fnt->pixels);
}

void fnt_initdraw(struct fontdraw *draw, unsigned char *img, int width, int height, int pitch)
{
	draw->img = img;
	draw->ptr = img;
	draw->width = width;
	draw->height = height;
	draw->pitch = pitch > 0 ? pitch : width;
	draw->curx = draw->cury = 0;
	draw->colorshift = 0;
	draw->colorbase = 0;
}

void fnt_position(struct fontdraw *draw, int x, int y)
{
	draw->curx = x;
	draw->cury = y;
	draw->ptr = draw->img + y * draw->pitch + x;
}

void fnt_drawglyph(struct font *fnt, struct fontdraw *draw, int c)
{
	int i, j;
	struct glyph *g;
	unsigned char *dest, *src;

	if(c < 0 || c >= 256) return;
	g = fnt->glyphs + c;
	if(!g->imgptr) return;

	dest = draw->ptr;
	src = g->imgptr;

	for(i=0; i<g->height; i++) {
		for(j=0; j<g->width; j++) {
			dest[j] = (src[j] >> draw->colorshift) + draw->colorbase;
		}
		dest += draw->pitch;
		src += fnt->xsz;
	}

	draw->ptr += g->advance;
}

void fnt_drawstr(struct font *fnt, struct fontdraw *draw, const char *str)
{
	while(*str) {
		fnt_drawglyph(fnt, draw, *(unsigned char*)str++);
	}
}

int fnt_strwidth(struct font *fnt, const char *str)
{
	int width = 0;
	while(*str) {
		width += fnt->glyphs[*(unsigned char*)str++].advance;
	}
	return width;
}
