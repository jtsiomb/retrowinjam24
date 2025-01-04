#include <stdio.h>
#include "scene.h"
#include "rend.h"
#include <float.h>
#include "dynarr.h"
#include "meshfile.h"
#include "imago2.h"

int init_scene(struct scene *scn)
{
	if(!(scn->meshes = dynarr_alloc(0, sizeof *scn->meshes))) {
		fprintf(stderr, "init_scene: failed to initialize meshes dynamic array\n");
		return -1;
	}
	if(!(scn->mtl = dynarr_alloc(0, sizeof *scn->mtl))) {
		fprintf(stderr, "init_scene: failed to initializes materials dynamic array\n");
		return -1;
	}
	return 0;
}

void destroy_scene(struct scene *scn)
{
	int i;

	for(i=0; i<dynarr_size(scn->meshes); i++) {
		free_mesh(scn->meshes[i]);
	}
	dynarr_free(scn->meshes);

	for(i=0; i<dynarr_size(scn->mtl); i++) {
		free_material(scn->mtl[i]);
	}
	dynarr_free(scn->mtl);
}

void free_material(struct material *mtl)
{
	if(!mtl) return;
	free(mtl->name);
	if(mtl->tex_diffuse) {
		free(mtl->tex_diffuse->pixels);
	}
	if(mtl->tex_normal) {
		free(mtl->tex_normal->pixels);
	}
	free(mtl);
}

void init_mesh(struct mesh *mesh)
{
	memset(mesh, 0, sizeof *mesh);
	cgm_midentity(mesh->xform);
	cgm_midentity(mesh->inv_xform);
}

void free_mesh(struct mesh *mesh)
{
	if(!mesh) return;
	free(mesh->faces);
}

int load_scene(struct scene *scn, const char *fname)
{
	int i, j, k;
	struct mf_meshfile *mf;
	struct mf_mesh *mfm;
	struct mf_material *mfmtl;
	struct mesh *mesh = 0;
	struct material *mtl = 0;
	const char *tex;
	cgm_vec3 va, vb;
	static const cgm_vec3 defnorm = {0, 1, 0};
	static const cgm_vec2 defuv;

	if(!(mf = mf_alloc()) || mf_load(mf, fname) == -1) {
		fprintf(stderr, "load_scene: failed to load: %s\n", fname);
		return -1;
	}

	for(i=0; i<mf_num_materials(mf); i++) {
		mfmtl = mf_get_material(mf, i);
		if(!(mtl = malloc(sizeof *mtl)) || !(mtl->name = strdup(mfmtl->name))) {
			fprintf(stderr, "load_scene: failed to allocate material\n");
			goto err;
		}
		mtl->kd = *(cgm_vec3*)&mfmtl->attr[MF_COLOR].val;
		mtl->ks = *(cgm_vec3*)&mfmtl->attr[MF_SPECULAR].val;
		mtl->shin = mfmtl->attr[MF_SHININESS].val.x;

		if(mfmtl->attr[MF_COLOR].map.name) {
			tex = mf_find_asset(mf, mfmtl->attr[MF_COLOR].map.name);
			if(!(mtl->tex_diffuse = load_texture(tex))) {
				goto err;
			}
		}
		if(mfmtl->attr[MF_BUMP].map.name) {
			tex = mf_find_asset(mf, mfmtl->attr[MF_BUMP].map.name);
			if(!(mtl->tex_normal = load_texture(tex))) {
				goto err;
			}
		}

		add_material(scn, mtl);
		mtl = 0;
	}

	for(i=0; i<mf_num_meshes(mf); i++) {
		mfm = mf_get_mesh(mf, i);

		if(!(mesh = malloc(sizeof *mesh))) {
			fprintf(stderr, "load_scene: failed to allocate mesh\n");
			goto err;
		}
		init_mesh(mesh);
		mesh->name = strdup(mfm->name);

		if(!(mesh->faces = malloc(mfm->num_faces * sizeof *mesh->faces))) {
			fprintf(stderr, "load_Scene: failed to allocate %u mesh faces\n", mfm->num_faces);
			goto err;
		}
		mesh->nfaces = mfm->num_faces;

		if(mfm->mtl) {
			mesh->mtl = find_material(scn, mfm->mtl->name);
		}

		for(j=0; j<mfm->num_faces; j++) {
			struct meshtri *tri = mesh->faces + j;

			for(k=0; k<3; k++) {
				int vidx = mfm->faces[j].vidx[k];
				tri->v[k].pos = *(cgm_vec3*)&mfm->vertex[vidx];
				tri->v[k].norm = mfm->normal ? *(cgm_vec3*)&mfm->normal[vidx] : defnorm;
				tri->v[k].uv = mfm->texcoord ? *(cgm_vec2*)&mfm->texcoord[vidx] : defuv;
			}

			va = tri->v[1].pos; cgm_vsub(&va, &tri->v[0].pos);
			vb = tri->v[2].pos; cgm_vsub(&vb, &tri->v[0].pos);
			cgm_vcross(&tri->norm, &va, &vb);
			cgm_vnormalize(&tri->norm);
		}

		mesh->aabb.vmin = *(cgm_vec3*)&mfm->aabox.vmin;
		mesh->aabb.vmax = *(cgm_vec3*)&mfm->aabox.vmax;

		add_mesh(scn, mesh);
		mesh = 0;
	}

	mf_free(mf);
	return 0;

err:
	free_material(mtl);
	free_mesh(mesh);
	mf_free(mf);
	return -1;
}

int dump_scene(struct scene *scn, const char *fname)
{
	int i, j, k;
	FILE *fp;
	long vidx = 0;
	struct mesh *mesh;
	struct meshtri *tri;

	if(!(fp = fopen(fname, "wb"))) {
		fprintf(stderr, "failed to open %s for writing\n", fname);
		return -1;
	}

	for(i=0; i<dynarr_size(scn->meshes); i++) {
		mesh = scn->meshes[i];

		fprintf(fp, "o %s\n", mesh->name);

		for(j=0; j<mesh->nfaces; j++) {
			tri = mesh->faces + j;
			for(k=0; k<3; k++) {
				fprintf(fp, "v %f %f %f\n", tri->v[k].pos.x, tri->v[k].pos.y, tri->v[k].pos.z);
			}
		}

		for(j=0; j<mesh->nfaces; j++) {
			tri = mesh->faces + j;
			for(k=0; k<3; k++) {
				fprintf(fp, "vn %f %f %f\n", tri->v[k].norm.x, tri->v[k].norm.y, tri->v[k].norm.z);
			}
		}

		for(j=0; j<mesh->nfaces; j++) {
			tri = mesh->faces + j;
			for(k=0; k<3; k++) {
				fprintf(fp, "vt %f %f\n", tri->v[k].uv.x, tri->v[k].uv.y);
			}
		}

		for(j=0; j<mesh->nfaces; j++) {
			tri = mesh->faces + j;
			fputc('f', fp);
			for(k=0; k<3; k++) {
				vidx++;
				fprintf(fp, " %ld/%ld/%ld", vidx, vidx, vidx);
			}
			fputc('\n', fp);
		}
	}

	fclose(fp);
	return 0;
}

void add_mesh(struct scene *scn, struct mesh *mesh)
{
	dynarr_push_ordie(scn->meshes, &mesh);
}

void add_material(struct scene *scn, struct material *mtl)
{
	dynarr_push_ordie(scn->mtl, &mtl);
}

struct material *find_material(struct scene *scn, const char *name)
{
	int i;

	for(i=0; i<dynarr_size(scn->mtl); i++) {
		if(strcmp(scn->mtl[i]->name, name) == 0) {
			return scn->mtl[i];
		}
	}
	return 0;
}

struct rendimage *load_texture(const char *fname)
{
	int width, height;
	float *img;
	struct rendimage *ri;

	if(!(img = img_load_pixels(fname, &width, &height, IMG_FMT_RGBAF))) {
		return 0;
	}

	if(!(ri = malloc(sizeof *ri))) {
		fprintf(stderr, "load_texture: failed to allocate image\n");
		return 0;
	}
	ri->pixels = (cgm_vec4*)img;
	ri->width = width;
	ri->height = height;
	return ri;
}

static void compute_rayhit(struct rayhit *hit, cgm_ray *ray)
{
	struct meshtri *tri = hit->face;

	hit->ray = *ray;
	cgm_raypos(&hit->pos, ray, hit->t);

	hit->norm.x = tri->v[0].norm.x * hit->bary.x + tri->v[1].norm.x * hit->bary.y +
		tri->v[2].norm.x * hit->bary.z;
	hit->norm.y = tri->v[0].norm.y * hit->bary.x + tri->v[1].norm.y * hit->bary.y +
		tri->v[2].norm.y * hit->bary.z;
	hit->norm.z = tri->v[0].norm.z * hit->bary.x + tri->v[1].norm.z * hit->bary.y +
		tri->v[2].norm.z * hit->bary.z;

	hit->tang.x = tri->v[0].tang.x * hit->bary.x + tri->v[1].tang.x * hit->bary.y +
		tri->v[2].tang.x * hit->bary.z;
	hit->tang.y = tri->v[0].tang.y * hit->bary.x + tri->v[1].tang.y * hit->bary.y +
		tri->v[2].tang.y * hit->bary.z;
	hit->tang.z = tri->v[0].tang.z * hit->bary.x + tri->v[1].tang.z * hit->bary.y +
		tri->v[2].tang.z * hit->bary.z;

	hit->uv.x = tri->v[0].uv.x * hit->bary.x + tri->v[1].uv.x * hit->bary.y +
		tri->v[2].uv.x * hit->bary.z;
	hit->uv.y = tri->v[0].uv.y * hit->bary.x + tri->v[1].uv.y * hit->bary.y +
		tri->v[2].uv.y * hit->bary.z;
}

int ray_scene(struct scene *scn, cgm_ray *ray, struct rayhit *hit)
{
	int i, count;
	struct rayhit tmphit, nearest = {FLT_MAX};

	count = dynarr_size(scn->meshes);

	if(!hit) {
		for(i=0; i<count; i++) {
			if(ray_mesh(scn->meshes[i], ray, 0)) {
				return 1;
			}
		}
		return 0;
	}

	for(i=0; i<count; i++) {
		if(ray_mesh(scn->meshes[i], ray, &tmphit) && tmphit.t < nearest.t) {
			nearest = tmphit;
		}
	}

	if(nearest.mesh) {
		*hit = nearest;
		compute_rayhit(hit, ray);
		return 1;
	}
	return 0;
}

int ray_mesh(struct mesh *mesh, cgm_ray *ray, struct rayhit *hit)
{
	int i;
	cgm_ray localray;
	struct rayhit tmphit, nearest = {FLT_MAX};

	localray = *ray;
	cgm_rmul_mr(&localray, mesh->inv_xform);

	if(!ray_aabb(&mesh->aabb, &localray)) {
		return 0;
	}

	if(!hit) {
		for(i=0; i<mesh->nfaces; i++) {
			if(ray_triangle(mesh->faces + i, &localray, 0)) {
				return 1;
			}
		}
		return 0;
	}

	for(i=0; i<mesh->nfaces; i++) {
		if(ray_triangle(mesh->faces + i, &localray, &tmphit) && tmphit.t < nearest.t) {
			nearest = tmphit;
		}
	}

	if(nearest.face) {
		*hit = nearest;
		return 1;
	}
	return 0;
}

#define EPSILON	1e-6
/*
int ray_triangle(struct meshtri *tri, cgm_ray *ray, struct rayhit *hit)
{
	cgm_vec3 vab, vac, pdir, qvec;
	float det, inv_det, t, v, w;

	vab = tri->v[1].pos; cgm_vsub(&vab, &tri->v[0].pos);
	vac = tri->v[2].pos; cgm_vsub(&vac, &tri->v[0].pos);

	if((det = -cgm_vdot(&ray->dir, &tri->norm)) < EPSILON && det > -EPSILON) {
		return 0;
	}

	//pdir = tri->v[0].pos; cgm_vsub(&pdir, &ray->origin);
	pdir = ray->origin; cgm_vsub(&pdir, &tri->v[0].pos);
	if((t = cgm_vdot(&pdir, &tri->norm)) < EPSILON || t >= det) {
		return 0;
	}

	cgm_vcross(&qvec, &ray->dir, &pdir);
	v = cgm_vdot(&vac, &qvec);
	if(v < 0.0f || v > det) return 0;

	w = cgm_vdot(&vab, &qvec);
	if(w < 0.0f || v + w > det) return 0;

	if(hit) {
		inv_det = 1.0f / det;
		v *= inv_det;
		w *= inv_det;

		hit->t = t * inv_det;
		hit->bary.x = 1.0f - v - w;
		hit->bary.y = v;
		hit->bary.z = w;
		hit->face = tri;
	}
	return 1;
}
*/

extern int dbgpixel;
int ray_triangle(struct meshtri *tri, cgm_ray *ray, struct rayhit *hit)
{
	float t, ndotdir;
	cgm_vec3 vdir, bc, pos;

	if(fabs(ndotdir = cgm_vdot(&ray->dir, &tri->norm)) <= EPSILON) {
		return 0;
	}

	vdir = tri->v[0].pos;
	cgm_vsub(&vdir, &ray->origin);

	if((t = cgm_vdot(&tri->norm, &vdir) / ndotdir) <= EPSILON || t > 1.0f) {
		return 0;
	}

	cgm_raypos(&pos, ray, t);
	cgm_bary(&bc, &tri->v[0].pos, &tri->v[1].pos, &tri->v[2].pos, &pos);

	if(bc.x < 0.0f || bc.x > 1.0f) return 0;
	if(bc.y < 0.0f || bc.y > 1.0f) return 0;
	if(bc.z < 0.0f || bc.z > 1.0f) return 0;

	if(hit) {
		hit->t = t;
		hit->bary = bc;
		hit->face = tri;
		hit->ray = *ray;
	}
	return 1;
}

#define SLABCHECK(dim)	\
	do { \
		invdir = 1.0f / ray->dir.dim;	\
		t0 = (box->vmin.dim - ray->origin.dim) * invdir;	\
		t1 = (box->vmax.dim - ray->origin.dim) * invdir;	\
		if(invdir < 0.0f) {	\
			tmp = t0;	\
			t0 = t1;	\
			t1 = tmp;	\
		}	\
		tmin = t0 > tmin ? t0 : tmin;	\
		tmax = t1 < tmax ? t1 : tmax;	\
		if(tmax < tmin) return 0; \
	} while(0)

int ray_aabb(struct aabox *box, cgm_ray *ray)
{
	float invdir, t0, t1, tmp;
	float tmin = 0.0f, tmax = 1.0f;

	SLABCHECK(x);
	SLABCHECK(y);
	SLABCHECK(z);

	return 1;
}
