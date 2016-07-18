#include <stdlib.h>
#include "piRenderer.h"

#include "opengl4x/piGL4X_Renderer.h"

namespace piLibs {


piRenderer *piRenderer::Create( const piRendererType type )
{
	if( type==GL ) return new piRendererGL4X();
	if( type==DX ) return NULL;

	return NULL;
}


piRenderer::piRenderer()
{
}

piRenderer::~piRenderer()
{
}


}