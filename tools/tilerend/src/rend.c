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
static void (*primray)(struct ray*, int, int);


static void raytrace(cgm_vec4 *color, struct ray *ray, struct scene *scn, int iter);
static void shade(cgm_vec4 *color, struct rayhit *hit, int iter);
static void perspray(struct ray *ray, int x, int y);
static void orthoray(struct ray *ray, int x, int y);


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
	struct ray ray;
	cgm_vec4 color;
	cgm_vec4 *pixels = rendfb->pixels;

	for(i=0; i<vp.height; i++) {
		for(j=0; j<vp.width; j++) {
			primray(&ray, j, i);
			raytrace(&color, &ray, scn, 0);
			*pixels++ = color;
		}
		pixels += rendfb->width - vp.width;
	}
}

static void raytrace(cgm_vec4 *color, struct ray *ray, struct scene *scn, int iter)
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
	float ndotl, ndoth, spec;
	cgm_vec3 hdir, vdir;
	cgm_vec3 ldir = {-0.707, 0, 0.707};
	struct material *mtl = hit->face->mtl;

	ndotl = cgm_vdot(&hit->norm, &ldir);
	if(ndotl < 0.0f) ndotl = 0.0f;

	vdir = hit->ray.dir;
	cgm_vcons(&vdir, -hit->ray.dir.x, -hit->ray.dir.y, -hit->ray.dir.z);
	cgm_vnormalize(&vdir);
	hdir = ldir; cgm_vadd(&hdir, &vdir);
	cgm_vnormalize(&hdir);

	ndoth = cgm_vdot(&hit->norm, &hdir);
	if(ndoth < 0.0f) ndoth = 0.0f;
	spec = pow(ndoth, mtl->shin);

	color->x = 0.05f + mtl->kd.x * ndotl + mtl->ks.x * spec;
	color->y = 0.05f + mtl->kd.y * ndotl + mtl->ks.y * spec;
	color->z = 0.05f + mtl->kd.z * ndotl + mtl->ks.z * spec;
	color->w = 1.0f;
}


static void perspray(struct ray *ray, int x, int y)
{
	ray->origin.x = ray->origin.y = ray->origin.z = 0.0f;

	ray->dir.x = ((float)x / (float)vp.width - 0.5f) * aspect;
	ray->dir.y = 0.5f - (float)y / (float)vp.height;
	ray->dir.z = -vpdist;
	cgm_vnormalize(&ray->dir);
	cgm_vscale(&ray->dir, raymag);

	cgm_vmul_m4v3(&ray->origin, view_xform);
	cgm_vmul_m3v3(&ray->dir, view_xform);

	ray->invdir.x = 1.0f / ray->dir.x;
	ray->invdir.y = 1.0f / ray->dir.y;
	ray->invdir.z = 1.0f / ray->dir.z;
}

static void orthoray(struct ray *ray, int x, int y)
{
	ray->origin.x = (((float)x / (float)vp.width) - 0.5f) * orthosz * aspect;
	ray->origin.y = -(((float)y / (float)vp.height) - 0.5f) * orthosz;
	ray->origin.z = zmin;

	ray->dir.x = ray->dir.z = 0;
	ray->dir.z = zmax - zmin;

	cgm_vmul_m4v3(&ray->origin, view_xform);
	cgm_vmul_m3v3(&ray->dir, view_xform);

	ray->invdir.x = 1.0f / ray->dir.x;
	ray->invdir.y = 1.0f / ray->dir.y;
	ray->invdir.z = 1.0f / ray->dir.z;
}
