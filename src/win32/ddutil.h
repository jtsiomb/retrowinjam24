#ifndef DDUTIL_H_
#define DDUTIL_H_

#include <ddraw.h>
#include "config.h"

#ifdef USE_DX5
typedef IDirectDrawSurface3 DDSurface;
#else
typedef IDirectDrawSurface DDSurface;
#endif

extern IDirectDraw2 *ddraw;
extern DDSurface *ddfront, *ddback;


void *ddlocksurf(DDSurface *surf, unsigned int *pitchret);

void ddblit(DDSurface *dest, RECT *drect, DDSurface *src, RECT *srect,
		unsigned int flags, DDBLTFX *fx);
void ddblitfast(DDSurface *dest, int x, int y, DDSurface *src, RECT *srect,
		unsigned int flags);

void ddflip(DDSurface *surf);

DDSurface *create_ddsurf(DDSURFACEDESC *ddsd);

const char *dderrstr(HRESULT err);


#endif	/* DDUTIL_H_ */
