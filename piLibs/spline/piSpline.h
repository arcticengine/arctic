#pragma once

namespace piLibs {

#define SPL_T1F_V1F     1
#define SPL_T1F_V2F     2
#define SPL_T1F_V3F     3
#define SPL_T1F_V4F     4
#define SPL_T1F_V5F     5

#define SPL_GET_POINT   0
#define SPL_GET_TANG    1

void piSpline( float *key, int num, int dim, int stride, float t, float *v, int derivada );

} // namespace piLibs