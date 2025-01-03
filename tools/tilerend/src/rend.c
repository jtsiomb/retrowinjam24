#include "rend.h"
#include "scene.h"

struct rendimage *rendfb;

static struct rect vp;
static float aspect;
static float orthosz, zmin, zmax;
static float view_xform[16];
static int max_iter = 5;


static void raytrace(cgm_vec4 *color, cgm_ray *ray, struct scene *scn, int iter);
static void shade(cgm_vec4 *color, struct rayhit *hit, int iter);
static void orthoray(cgm_ray *ray, int x, int y);


int rend_init(void)
{
}

void rend_cleanup(void)
{
}

void rend_viewport(int x, int y, int width, int height)
{
	vp.x = x;
	vp.y = y;
	vp.width = width;
	vp.height = height;

	aspect = (float)width / (float)height;
}

void rend_ortho(float ysz, float z0, float z1)
{
	zmin = z0;
	zmax = z1;
	orthosz = ysz;
}

void rend_view(float *xform)
{
	memcpy(view_xform, xform, sizeof view_xform);
}

void render(struct scene *scn)
{
	int i, j;
	cgm_ray ray;
	cgm_vec4 color;

	for(i=0; i<vp.height; i++) {
		for(j=0; j<vp.width; j++) {
			orthoray(&ray, j, i);
			raytrace(&color, &ray, scn, 0);
		}
	}
}

static void raytrace(cgm_vec4 *color, cgm_ray *ray, struct scene *scn, int iter)
{
	struct rayhit hit;

	if(iter >= max_iter || !ray_scene(scn, ray, &hit)) {
		color->x = color->y = color->z = color->w = 0;
	} else {
		shade(color, &hit, iter);
	}
}

static void shade(cgm_vec4 *color, struct rayhit *hit, int iter)
{
	color->x = 1;
	color->y = color->z = 0;
	color->w = 1;
}


static void orthoray(cgm_ray *ray, int x, int y)
{
	ray->origin.x = (((float)x / (float)vp.width) - 0.5f) * orthosz * aspect;
	ray->origin.y = -(((float)y / (float)vp.height) - 0.5f) * orthosz;
	ray->origin.z = zmin;

	ray->dir.x = ray->dir.z = 0;
	ray->dir.z = zmax - zmin;

	cgm_vmul_m4v3(&ray->origin, view_xform);
	cgm_vmul_m3v3(&ray->dir, view_xform);
}
