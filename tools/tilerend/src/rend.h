#ifndef REND_H_
#define REND_H_

#include "cgmath/cgmath.h"

struct scene;

struct rect {
	int x, y, width, height;
};

struct rendimage {
	int width, height;
	cgm_vec4 *pixels;
};

extern struct rendimage *rendfb;

int rend_init(void);
void rend_cleanup(void);

void rend_viewport(int x, int y, int width, int height);
void rend_ortho(float ysz, float zmin, float zmax);
void rend_view(float *xform);

void render(struct scene *scn);

#endif	/* REND_H_ */
