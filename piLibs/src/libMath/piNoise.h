#pragma once

#include "piVecTypes.h"

namespace piLibs {


float piNoise_1f_Grad_Hash( const float x );
vec2  piNoise_2f_Grad_Hash( const vec2 & x, const int pe=255 );
float piNoise_1f_Grad_Hash( const vec3 & x );
vec3  piNoise_3f_Grad_Hash( const vec3 & x );
float piNoise_1f_Grad_Lut( const float x );

vec3  piNoise_3f_Value_Hash( const vec3 & x );
float piNoise_1f_Value_Hash( const float x );
vec2  piNoise_2f_Value_Hash( const float x );
vec3  piNoise_3f_Value_Hash( const float x );
float piNoise_1f_Value_Hash( const vec2 & x );
float piNoise_1f_Value_Hash( const vec2 & x );
vec3  piNoise_3f_Value_Hash( const vec2 & x );
float piNoise_1f_Value_Hash( const vec3 & x );
vec2  piNoise_2f_Value_Hash( const vec3 & x );
vec3  piNoise_3f_Value_Hash( const vec3 & x );

} // namespace piLibs