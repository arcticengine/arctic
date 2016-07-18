#ifndef _PFM_H_
#define _PFM_H_

#include "image.h"

int PFM_Load( IMAGE *bmp, const char *name );
int PFM_Save( IMAGE *bmp, const char *name );

#endif
