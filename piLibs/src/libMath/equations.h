#pragma once

namespace piLibs {

int solveCuadratic( float b, float c, float *res );
int solveCubic( float b, float c, float d, float *res )
int solveQuartic( float b, float c, float d, float e, float *res );


// solveCubic() calculates the 3 roots of the cubic. Returns the number of real roots (1 or 3)
// 1 real root:
//    res[0] = real root 
//    res[2] = Re{root 2}
//    res[3] = Im{root 2}
//    res[4] = Re{root 3}
//    res[5] = Im{root 3}
// 3 real roots
//    res[0] = real root 1
//    res[2] = real root 2
//    res[4] = real root 3

} // namespace piLibs