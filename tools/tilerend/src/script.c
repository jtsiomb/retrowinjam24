#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <alloca.h>
#include <sys/time.h>
#include "tilerend.h"

static char *clean_line(char *s);
static char *next_token(char **line);

static int cmd_load(char *args);
static int cmd_image(char *args);
static int cmd_imagesize(char *args);
static int cmd_tilesize(char *args);
static int cmd_viewpos(char *args);
static int cmd_viewtarg(char *args);
static int cmd_persp(char *args);
static int cmd_ortho(char *args);
static int cmd_render(char *args);

static struct {
	const char *name;
	int (*func)(char*);
} cmdtab[] = {
	{"load", cmd_load},
	{"image", cmd_image},
	{"imagesize", cmd_imagesize},
	{"tilesize", cmd_tilesize},
	{"viewpos", cmd_viewpos},
	{"viewtarg", cmd_viewtarg},
	{"persp", cmd_persp},
	{"ortho", cmd_ortho},
	{"render", cmd_render},
	{0, 0}
};

static int tilex, tiley;
static int fov = 50;
static float orthosz, orthomin, orthomax;

int parse_cmd(const char *cmdline)
{
	int i;
	char *s;
	char *line = alloca(strlen(cmdline) + 1);
	strcpy(line, cmdline);

	if((s = strchr(line, '#'))) {
		*s = 0;
	}

	if(!(s = next_token(&line))) return 0;

	for(i=0; cmdtab[i].name; i++) {
		if(strcmp(cmdtab[i].name, s) == 0) {
			return cmdtab[i].func(clean_line(line));
		}
	}

	fprintf(stderr, "unknown command: %s\n", s);
	return -1;
}

static char *clean_line(char *s)
{
	char *end;

	while(*s && isspace(*s)) ++s;
	if(!*s) return 0;

	end = s;
	while(*end) ++end;
	*end-- = 0;

	while(end > s && isspace(*end)) {
		*end-- = 0;
	}
	return s;
}

static char *next_token(char **line)
{
	char *tok, *s = *line;

	while(*s && isspace(*s)) s++;
	if(!*s) {
		*line = 0;
		return 0;
	}

	tok = s;
	while(*s && !isspace(*s)) s++;

	if(!*s) {
		*line = 0;
	} else {
		*line = s + 1;
	}
	*s = 0;
	return tok;
}


static int cmd_load(char *args)
{
	args = clean_line(args);

	if(!args) {
		fprintf(stderr, "missing filename from load command\n");
		return -1;
	}

	if(load_scene(&scn, args) == -1) {
		return -1;
	}
	return 0;
}

static int cmd_image(char *args)
{
	free(outfname);
	outfname = strdup(args);
	return outfname != 0;
}

static int cmd_imagesize(char *args)
{
	if(!args) {
		fprintf(stderr, "missing argument from imagesize command\n");
		return -1;
	}

	if(sscanf(args, "%dx%d", &framebuf.width, &framebuf.height) != 2) {
		fprintf(stderr, "invalid argument to imagesize: %s\n", args);
		return -1;
	}
	return 0;
}

static int cmd_tilesize(char *args)
{
	if(!args) {
		fprintf(stderr, "missing argument from tilesize command\n");
		return -1;
	}

	if(sscanf(args, "%dx%d", &tilewidth, &tileheight) != 2) {
		fprintf(stderr, "invalid argument to tilesize: %s\n", args);
		return -1;
	}
	return 0;
}

static int cmd_viewpos(char *args)
{
	if(!args) {
		fprintf(stderr, "missing argument from viewpos command\n");
		return -1;
	}

	if(sscanf(args, "%f %f %f", &viewpos.x, &viewpos.y, &viewpos.z) != 3) {
		fprintf(stderr, "invalid argument to viewpos: %s\n", args);
		return -1;
	}
	return 0;
}

static int cmd_viewtarg(char *args)
{
	if(!args) {
		fprintf(stderr, "missing argument from viewtarg command\n");
		return -1;
	}

	if(sscanf(args, "%f %f %f", &viewtarg.x, &viewtarg.y, &viewtarg.z) != 3) {
		fprintf(stderr, "invalid argument to viewtarg: %s\n", args);
		return -1;
	}
	return 0;
}

static int cmd_persp(char *args)
{
	int val;

	if(!args) {
		fprintf(stderr, "missing argument from persp command\n");
		return -1;
	}

	if((val = atoi(args)) <= 0 || val >= 180) {
		fprintf(stderr, "invalid argument to persp: %s\n", args);
		return -1;
	}
	fov = cgm_deg_to_rad(val);
	orthosz = 0;
	return 0;
}

static int cmd_ortho(char *args)
{
	if(!args) {
		fprintf(stderr, "misisng argument from ortho command\n");
		return -1;
	}

	if(sscanf(args, "%f %f %f", &orthosz, &orthomin, &orthomax) != 3 || orthosz <= 0) {
		fprintf(stderr, "invalid arguments to ortho: %s\n", args);
		return -1;
	}
	if(orthomax < orthomin) {
		float tmp = orthomin;
		orthomin = orthomax;
		orthomax = tmp;
	}
	fov = 0;
	return 0;
}

static int cmd_render(char *args)
{
	float xform[16];
	struct timeval tv, tv0;
	cgm_vec3 up = {0, 1, 0};

	if(!framebuf.pixels) {
		if(!(framebuf.pixels = malloc(framebuf.width * framebuf.height * sizeof *framebuf.pixels))) {
			fprintf(stderr, "failed to allocate %dx%d framebuffer\n", framebuf.width, framebuf.height);
			return -1;
		}
		rendfb = &framebuf;
		tilex = tiley = 0;
	}

	if(tiley + tileheight > framebuf.height) {
		fprintf(stderr, "render failed, tile image full\n");
		return -1;
	}

	rend_viewport(tilex, tiley, tilewidth, tileheight);
	if(orthosz) {
		rend_ortho(orthosz, orthomin, orthomax);
	} else {
		float maxdist = scn.bsph_rad * 16.0f;
		float fovrad = cgm_deg_to_rad(fov);
		rend_perspective(fovrad, maxdist ? maxdist : 100.0f);
	}

	if(fabs(viewtarg.y - viewpos.y) >= 0.98) {
		cgm_vcons(&up, 0, 0, 1);
	}
	cgm_mlookat(xform, &viewpos, &viewtarg, &up);
	rend_view(xform);

	printf("render %dx%d at %d,%d ... ", tilewidth, tileheight, tilex, tiley);
	fflush(stdout);

	gettimeofday(&tv0, 0);
	render(&scn);
	gettimeofday(&tv, 0);

	printf("%g sec\n", (float)(tv.tv_sec - tv0.tv_sec) + (float)(tv.tv_usec - tv0.tv_usec) / 1000000.0f);

	tilex += tilewidth;
	if(tilex + tilewidth > framebuf.width) {
		tilex = 0;
		tiley += tileheight;
	}
	return 0;
}
