#ifndef SCENE_H_
#define SCENE_H_

#include "cgmath/cgmath.h"

struct rendimage;

struct material {
	char *name;
	cgm_vec3 kd, ks;
	float shin;
	struct rendimage *tex_diffuse;
	struct rendimage *tex_normal;
};

struct aabox {
	cgm_vec3 vmin, vmax;
};

struct vertex {
	cgm_vec3 pos, norm;
	cgm_vec2 uv;
};

struct meshtri {
	struct vertex v[3];
	cgm_vec3 norm;
};

struct mesh {
	struct meshtri *faces;
	int nfaces;

	struct material *mtl;
};

struct scene {
	struct mesh **meshes;
	struct material **mtl;
};

struct rayhit {
	float t;
	cgm_ray ray;
	struct mesh *mesh;
	struct meshtri *face;
	cgm_vec3 bary;

	cgm_vec3 pos, norm;
	cgm_vec2 uv;
};

void free_material(struct material *mtl);
void free_mesh(struct mesh *mesh);

int load_scene(struct scene *scn, const char *fname);

void add_mesh(struct scene *scn, struct mesh *mesh);
void add_material(struct scene *scn, struct material *mtl);

struct material *find_material(struct scene *scn, const char *name);

struct rendimage *load_texture(const char *fname);

int ray_scene(struct scene *scn, cgm_ray *ray, struct rayhit *hit);
int ray_mesh(struct mesh *mesh, cgm_ray *ray, struct rayhit *hit);
int ray_triangle(struct meshtri *tri, cgm_ray *ray, struct rayhit *hit);

int ray_aabb(struct aabox *box, cgm_ray *ray);

#endif /* SCENE_H_ */
