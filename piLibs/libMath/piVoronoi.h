#pragma once

namespace piLibs {

void piVoronoi_2f_1f( const float *x, float *resD1, float *resP1, int *resID1, 
                                      float *resD2, float *resP2, int *resID2, 
                                      unsigned int pe );

} // namespace piLibs