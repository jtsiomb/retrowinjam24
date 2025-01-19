#include <stdio.h>
#include "rend.h"
#include "scene.h"
#include "dynarr.h"

int dbgpixel;

struct rendimage *rendfb;

static struct rect vp;
static float aspect;
static float vpdist, raymag, orthosz, zmin, zmax;
static float view_xform[16];
static int max_iter = 5;
static void (*primray)(struct ray*, int, int);


static void raytrace(cgm_vec4 *color, struct ray *ray, struct scene *scn, int iter);
static void shade(cgm_vec4 *color, struct scene *scn, struct rayhit *hit, int iter);
static void texlookup(cgm_vec4 *color, struct rendimage *img, cgm_vec2 uv);
static void calc_invdir(struct ray *ray);
static void perspray(struct ray *ray, int x, int y);
static void orthoray(struct ray *ray, int x, int y);


int rend_init(void)
{
	rend_perspective(cgm_deg_to_rad(50), 1000.0f);
	cgm_midentity(view_xform);
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
	cgm_vec4 *pixels = rendfb->pixels + vp.y * rendfb->width + vp.x;

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
		shade(color, scn, &hit, iter);
	}
}

static void cons_tbn_matrix(float *tbn, struct rayhit *hit)
{
	cgm_vec3 bitan;
	cgm_vcross(&bitan, &hit->tang, &hit->norm);

	/*
	tbn[0] = hit->tang.x;
	tbn[4] = hit->tang.y;
	tbn[8] = hit->tang.z;
	tbn[1] = bitan.x;
	tbn[5] = bitan.y;
	tbn[9] = bitan.z;
	tbn[2] = hit->norm.x;
	tbn[6] = hit->norm.y;
	tbn[10] = hit->norm.z;
	*/
	tbn[0] = hit->tang.x;
	tbn[1] = hit->tang.y;
	tbn[2] = hit->tang.z;
	tbn[4] = bitan.x;
	tbn[5] = bitan.y;
	tbn[6] = bitan.z;
	tbn[8] = hit->norm.x;
	tbn[9] = hit->norm.y;
	tbn[10] = hit->norm.z;
	tbn[3] = tbn[7] = tbn[11] = tbn[12] = tbn[13] = tbn[14] = 0.0f;
	tbn[15] = 1.0f;
}

static void shade(cgm_vec4 *color, struct scene *scn, struct rayhit *hit, int iter)
{
	int i, numlights;
	float ndotl, ndoth, spec;
	cgm_vec3 norm, hdir, vdir, ldir;
	struct material *mtl = hit->face->mtl;
	struct light *lt;
	struct ray sray;
	cgm_vec3 basecol;
	cgm_vec4 texel;
	float tbn[16];

	if(mtl->tex_normal) {
		cons_tbn_matrix(tbn, hit);
		texlookup(&texel, mtl->tex_normal, hit->uv);

		cgm_vcons(&norm, texel.x * 2.0f - 1.0f, texel.y * 2.0f - 1.0f, texel.z * 2.0f - 1.0f);
		cgm_vmul_m3v3(&norm, tbn);
	} else {
		norm = hit->norm;
	}

	if(mtl->tex_diffuse) {
		texlookup(&texel, mtl->tex_diffuse, hit->uv);
		basecol = *(cgm_vec3*)&texel;
	} else {
		basecol = mtl->kd;
	}
	color->x = basecol.x * 0.05f;
	color->y = basecol.y * 0.05f;
	color->z = basecol.z * 0.05f;
	color->w = 1.0f;

	numlights = dynarr_size(scn->lights);
	for(i=0; i<numlights; i++) {
		lt = scn->lights[i];

		switch(lt->type) {
		case PTLIGHT:
			ldir = lt->posdir;
			cgm_vsub(&ldir, &hit->pos);
			break;

		case DIRLIGHT:
			ldir = lt->posdir;
			break;

		default:
			continue;	/* ignore invalid light type */
		}

		if(lt->shadows) {
			sray.origin = hit->pos;
			sray.dir = ldir;
			if(lt->type == DIRLIGHT) {
				cgm_vscale(&sray.dir, scn->bsph_rad * 4.0f);
			}
			calc_invdir(&sray);

			if(ray_scene(scn, &sray, 0)) {
				continue;
			}
		}

		if(lt->type == PTLIGHT) {
			cgm_vnormalize(&ldir);
		}

		ndotl = cgm_vdot(&norm, &ldir);
		if(ndotl < 0.0f) ndotl = 0.0f;

		vdir = hit->ray.dir;
		cgm_vcons(&vdir, -hit->ray.dir.x, -hit->ray.dir.y, -hit->ray.dir.z);
		cgm_vnormalize(&vdir);
		hdir = ldir; cgm_vadd(&hdir, &vdir);
		cgm_vnormalize(&hdir);

		ndoth = cgm_vdot(&norm, &hdir);
		if(ndoth < 0.0f) ndoth = 0.0f;
		spec = pow(ndoth, mtl->shin);

		color->x += basecol.x * ndotl + mtl->ks.x * spec;
		color->y += basecol.y * ndotl + mtl->ks.y * spec;
		color->z += basecol.z * ndotl + mtl->ks.z * spec;
	}
}

static void texlookup(cgm_vec4 *color, struct rendimage *img, cgm_vec2 uv)
{
	int tx = (int)(fmod(uv.x, 1.0f) * (float)img->width);
	int ty = (int)(fmod(uv.y, 1.0f) * (float)img->height);
	*color = img->pixels[ty * img->width + tx];
}

static void calc_invdir(struct ray *ray)
{
	ray->invdir.x = ray->dir.x == 0.0f ? 0.0f : 1.0f / ray->dir.x;
	ray->invdir.y = ray->dir.y == 0.0f ? 0.0f : 1.0f / ray->dir.y;
	ray->invdir.z = ray->dir.z == 0.0f ? 0.0f : 1.0f / ray->dir.z;
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

	calc_invdir(ray);
}

static void orthoray(struct ray *ray, int x, int y)
{
	ray->origin.x = ((float)x / (float)vp.width - 0.5f) * orthosz * aspect;
	ray->origin.y = (0.5f - (float)y / (float)vp.height) * orthosz;
	ray->origin.z = zmax;

	ray->dir.x = ray->dir.y = 0;
	ray->dir.z = zmin - zmax;

	cgm_vmul_m4v3(&ray->origin, view_xform);
	cgm_vmul_m3v3(&ray->dir, view_xform);

	calc_invdir(ray);
}
