#pragma once

namespace piLibs {

void piFft1( unsigned  int log2NumSamples, int InverseTransform, const float *RealIn, const float *ImagIn, 
            float *RealOut, float *ImagOut );


/*
Replaces data[1..2*nn] by its discrete Fourier transform, if isign is input as 1; or replaces
data[1..2*nn] by nn times its inverse discrete Fourier transform, if isign is input as −1.
data is a complex array of length nn or, equivalently, a real array of length 2*nn. nn MUST
be an integer power of 2 (this is not checked for!).
*/
void piFft2(float *data, unsigned long nn, int isign);

} // namespace piLibs
