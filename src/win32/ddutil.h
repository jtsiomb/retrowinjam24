#ifndef DDUTIL_H_
#define DDUTIL_H_

#include <ddraw.h>

extern IDirectDraw2 *ddraw;
extern IDirectDrawSurface *ddfront, *ddback;


void *ddlocksurf(IDirectDrawSurface *surf, long *pitchret);

void ddblit(IDirectDrawSurface *dest, RECT *drect, IDirectDrawSurface *src,
			RECT *srect, unsigned int flags, DDBLTFX *fx);
void ddblitfast(IDirectDrawSurface *dest, int x, int y, IDirectDrawSurface *src,
			RECT *srect, unsigned int flags);

void ddflip(IDirectDrawSurface *surf);

IDirectDrawSurface2 *create_ddsurf2(DDSURFACEDESC *ddsd);

const char *dderrstr(HRESULT err);


#endif	/* DDUTIL_H_ */
