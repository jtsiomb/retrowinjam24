#include <stdio.h>
#include <stdlib.h>
#include "rend.h"
#include "scene.h"
#include "cgmath/cgmath.h"
#include "imago2.h"

static struct scene scn;
static float view_xform[16];
static const char *infname;
static struct rendimage fb;

static int parse_args(int argc, char **argv);


int main(int argc, char **argv)
{
	if(parse_args(argc, argv) == -1) {
		return 1;
	}

	if(!infname) {
		fprintf(stderr, "you need to specify a mesh file to load\n");
		return 1;
	}
	if(init_scene(&scn) == -1 || load_scene(&scn, infname) == -1) {
		return 1;
	}
	dump_scene(&scn, "foo.obj");

	cgm_midentity(view_xform);

	rend_init();

	fb.width = 512;
	fb.height = 512;
	if(!(fb.pixels = malloc(fb.width * fb.height * sizeof *fb.pixels))) {
		abort();
	}
	rendfb = &fb;

	//rend_viewport(0, 0, 128, 64);
	//rend_ortho(4, -100, 100);
	rend_viewport(0, 0, 256, 256);
	rend_perspective(cgm_deg_to_rad(60), 100.0f);
	rend_view(view_xform);

	render(&scn);

	if(img_save_pixels("output.png", fb.pixels, fb.width, fb.height,IMG_FMT_RGBAF) == -1) {
		fprintf(stderr, "failed to write output image\n");
		return 1;
	}
	return 0;
}

static const char *usage = "usage %s [options] <file>\n"
	"Options:\n"
	" -h,-help: print usage and exit\n";

static int parse_args(int argc, char **argv)
{
	int i;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0) {
				printf(usage, argv[0]);
				exit(0);
			} else {
				fprintf(stderr, "invalid option: %s\n", argv[i]);
				return -1;
			}
		} else {
			if(infname) {
				fprintf(stderr, "unexpected argument: %s\n", argv[i]);
				return -1;
			}
			infname = argv[i];
		}
	}

	return 0;
}
