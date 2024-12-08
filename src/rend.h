#ifndef REND_H_
#define REND_H_

#include "level.h"

enum {
	REND_SHADE = 1,
	REND_HOVER = 2
};

struct renderstate {
	unsigned int flags;
	vec2i pan, pan_min, pan_max;
	vec2i hovertile;

	int stat_nblits, stat_ntiles;
};

extern struct renderstate rend;

void render_init(void);
void reset_view(void);
void pan_view(int dx, int dy);

void render_view(void);

#endif	/* REND_H_ */
