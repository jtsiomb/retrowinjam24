#ifndef TILEREND_H_
#define TILEREND_H_

#include "cgmath/cgmath.h"
#include "rend.h"
#include "scene.h"

extern struct scene scn;
extern char *outfname;
extern struct rendimage framebuf;
extern int tilewidth, tileheight;
extern cgm_vec3 viewpos, viewtarg;

#endif	/* TILEREND_H_ */
