#include <stdio.h>
#include "scene.h"
#include "rend.h"
#include <float.h>
#include "dynarr.h"
#include "meshfile.h"
#include "imago2.h"

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
		if(!(mesh->faces = malloc(mfm->num_faces * sizeof *mesh->faces))) {
			fprintf(stderr, "load_Scene: failed to allocate %u mesh faces\n", mfm->num_faces);
			goto err;
		}

		for(j=0; j<mfm->num_faces; j++) {
			struct meshtri *tri = mesh->faces + i;

			for(k=0; k<3; k++) {
				int vidx = mfm->faces[i].vidx[k];
				tri->v[k].pos = *(cgm_vec3*)&mfm->vertex[vidx];
				tri->v[k].norm = mfm->normal ? *(cgm_vec3*)&mfm->normal[vidx] : defnorm;
				tri->v[k].uv = mfm->texcoord ? *(cgm_vec2*)&mfm->texcoord[vidx] : defuv;
			}

			va = tri->v[1].pos; cgm_vsub(&va, &tri->v[0].pos);
			vb = tri->v[2].pos; cgm_vsub(&vb, &tri->v[0].pos);
			cgm_vcross(&tri->norm, &va, &vb);
			cgm_vnormalize(&tri->norm);
		}

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
		return 1;
	}
	return 0;
}

int ray_mesh(struct mesh *mesh, cgm_ray *ray, struct rayhit *hit)
{
	int i;
	struct rayhit tmphit, nearest = {FLT_MAX};

	if(!hit) {
		for(i=0; i<mesh->nfaces; i++) {
			if(ray_triangle(mesh->faces + i, ray, 0)) {
				return 1;
			}
		}
		return 0;
	}

	for(i=0; i<mesh->nfaces; i++) {
		if(ray_triangle(mesh->faces + i, ray, &tmphit) && tmphit.t < nearest.t) {
			nearest = tmphit;
		}
	}

	if(nearest.face) {
		*hit = nearest;
		return 1;
	}
	return 0;
}

int ray_triangle(struct meshtri *tri, cgm_ray *ray, struct rayhit *hit)
{
}

int ray_aabb(struct aabox *box, cgm_ray *ray)
{
}
