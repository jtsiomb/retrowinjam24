#include <stdio.h>
#include "rend.h"
#include "scene.h"

int dbgpixel;

struct rendimage *rendfb;

static struct rect vp;
static float aspect;
static float vpdist, raymag, orthosz, zmin, zmax;
static float view_xform[16];
static int max_iter = 5;
static void (*primray)(cgm_ray*, int, int);


static void raytrace(cgm_vec4 *color, cgm_ray *ray, struct scene *scn, int iter);
static void shade(cgm_vec4 *color, struct rayhit *hit, int iter);
static void perspray(cgm_ray *ray, int x, int y);
static void orthoray(cgm_ray *ray, int x, int y);


int rend_init(void)
{
	rend_perspective(cgm_deg_to_rad(50), 1000.0f);
	return 0;
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

void rend_perspective(float fov, float zfar)
{
	primray = perspray;
	vpdist = 1.0f / tan(fov * 0.5f);
	raymag = zfar;
}

void rend_ortho(float ysz, float z0, float z1)
{
	zmin = z0;
	zmax = z1;
	orthosz = ysz;
	primray = orthoray;
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
	cgm_vec4 *pixels = rendfb->pixels;

	for(i=0; i<vp.height; i++) {
		for(j=0; j<vp.width; j++) {
			dbgpixel = (i == vp.height/2 && j == vp.width/2);

			primray(&ray, j, i);
			raytrace(&color, &ray, scn, 0);
			*pixels++ = color;
		}
		pixels += rendfb->width - vp.width;
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


static void perspray(cgm_ray *ray, int x, int y)
{
	ray->origin.x = ray->origin.y = ray->origin.z = 0.0f;

	ray->dir.x = ((float)x / (float)vp.width - 0.5f) * aspect * raymag;
	ray->dir.y = (0.5f - (float)y / (float)vp.height) * raymag;
	ray->dir.z = -vpdist * raymag;

	cgm_rmul_mr(ray, view_xform);
}

static void orthoray(cgm_ray *ray, int x, int y)
{
	ray->origin.x = (((float)x / (float)vp.width) - 0.5f) * orthosz * aspect;
	ray->origin.y = -(((float)y / (float)vp.height) - 0.5f) * orthosz;
	ray->origin.z = zmin;

	ray->dir.x = ray->dir.z = 0;
	ray->dir.z = zmax - zmin;

	cgm_rmul_mr(ray, view_xform);
}
