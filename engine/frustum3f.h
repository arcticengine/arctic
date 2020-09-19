// The MIT License (MIT)
//
// Copyright (c) 2015 - 2016 Inigo Quilez
// Copyright (c) 2016 Huldra
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#ifndef ENGINE_FRUSTUM3F_H_
#define ENGINE_FRUSTUM3F_H_

#include <cmath>
#include "engine/arctic_types.h"
#include "engine/vec4f.h"

namespace arctic {

/// @addtogroup global_math
/// @{

struct Frustum3F {
  Vec4F planes[6];
  Vec3F points[8];
  Mat44F matrix;

  Frustum3F() {}
  /*
  // build a frustum from 6 mPlanes[anes
  explicit frustum3( vec4 const & a, vec4 const & b,
  vec4 const & c, vec4 const & d,
  vec4 const & e, vec4 const & f )
  {
  mPlanes[0] = a;
  mPlanes[1] = b;
  mPlanes[2] = c;
  mPlanes[3] = d;
  mPlanes[4] = e;
  mPlanes[5] = f;
  }

  // build a frustum from its 8 corner points
  explicit frustum3( vec3 const & v0, vec3 const & v1,
  vec3 const & v2, vec3 const & v3,
  vec3 const & v4, vec3 const & v5,
  vec3 const & v6, vec3 const & v7 )
  {
  mPoints[0] = v0;
  mPoints[1] = v1;
  mPoints[2] = v2;
  mPoints[3] = v3;
  mPoints[4] = v4;
  mPoints[5] = v5;
  mPoints[6] = v6;
  mPoints[7] = v7;

  mPlanes[0] = makemPlanes[ane( v0, v1, v2 );
  mPlanes[1] = makemPlanes[ane( v7, v6, v5 );
  mPlanes[2] = makemPlanes[ane( v1, v5, v6 );
  mPlanes[3] = makemPlanes[ane( v0, v3, v7 );
  mPlanes[4] = makemPlanes[ane( v3, v2, v6 );
  mPlanes[5] = makemPlanes[ane( v0, v4, v5 );
  }
  */

  explicit Frustum3F(Mat44F const &m) {
    matrix = m;

    planes[0].x = matrix[12] - matrix[0];
    planes[0].y = matrix[13] - matrix[1];
    planes[0].z = matrix[14] - matrix[2];
    planes[0].w = matrix[15] - matrix[3];
    planes[1].x = matrix[12] + matrix[0];
    planes[1].y = matrix[13] + matrix[1];
    planes[1].z = matrix[14] + matrix[2];
    planes[1].w = matrix[15] + matrix[3];
    planes[2].x = matrix[12] + matrix[4];
    planes[2].y = matrix[13] + matrix[5];
    planes[2].z = matrix[14] + matrix[6];
    planes[2].w = matrix[15] + matrix[7];
    planes[3].x = matrix[12] - matrix[4];
    planes[3].y = matrix[13] - matrix[5];
    planes[3].z = matrix[14] - matrix[6];
    planes[3].w = matrix[15] - matrix[7];
    planes[4].x = matrix[12] - matrix[8];
    planes[4].y = matrix[13] - matrix[9];
    planes[4].z = matrix[14] - matrix[10];
    planes[4].w = matrix[15] - matrix[11];
    planes[5].x = matrix[12] + matrix[8];
    planes[5].y = matrix[13] + matrix[9];
    planes[5].z = matrix[14] + matrix[10];
    planes[5].w = matrix[15] + matrix[11];

    for (Si32 i = 0; i < 6; i++) {
      planes[i] *= InverseLength(planes[i].xyz());
    }

    // same as bellow, just that *zar/near
    points[0] = IntersectPlanes(planes[1], planes[2], planes[4]);
    points[1] = IntersectPlanes(planes[0], planes[2], planes[4]);
    points[2] = IntersectPlanes(planes[0], planes[3], planes[4]);
    points[3] = IntersectPlanes(planes[1], planes[3], planes[4]);
    // left, bottom, near = -right, -top, near
    points[4] = IntersectPlanes(planes[1], planes[2], planes[5]);
    // right, bottom, near = right, -top, near
    points[5] = IntersectPlanes(planes[0], planes[2], planes[5]);
    // right, top, near
    points[6] = IntersectPlanes(planes[0], planes[3], planes[5]);
    // left, top, near = -right, top, near
    points[7] = IntersectPlanes(planes[1], planes[3], planes[5]);
  }
};

inline Vec3F GetNearPoint(Frustum3F const &fru, const Vec2F &uv) {
  return Mix(Mix(fru.points[4], fru.points[5], uv.x),
    Mix(fru.points[7], fru.points[6], uv.x), uv.y);
}

inline Frustum3F SetFrustum(float left, float right, float bottom, float top,
  float znear, float zfar) {
  const float x = (2.0f * znear) / (right - left);
  const float y = (2.0f * znear) / (top - bottom);
  const float a = (right + left) / (right - left);
  const float b = (top + bottom) / (top - bottom);
  const float c = -(zfar + znear) / (zfar - znear);
  const float d = -(2.0f * zfar * znear) / (zfar - znear);

  return Frustum3F(Mat44F(x, 0.0f, a, 0.0f,
    0.0f, y, b, 0.0f,
    0.0f, 0.0f, c, d,
    0.0f, 0.0f, -1.0f, 0.0f));
}

inline Frustum3F SetFrustumPerspective(float fovy, float aspect,
  float znear, float zfar) {
  const float ymax = znear * tanf(fovy * 3.141592653589f / 180.0f);
  const float ymin = -ymax;
  const float xmin = ymin * aspect;
  const float xmax = ymax * aspect;

  return SetFrustum(xmin, xmax, ymin, ymax, znear, zfar);
}

inline Frustum3F SetFrustumProjection(const Vec4F &fov,
  float znear, float zfar) {
  const float ymax = znear * fov.x;
  const float ymin = -znear * fov.y;
  const float xmin = -znear * fov.z;
  const float xmax = znear * fov.w;

  return SetFrustum(xmin, xmax, ymin, ymax, znear, zfar);
}

/*
inline frustum3 setPersectiveCheap( float fovy, float aspect,
float znear, float zfar, float expansion=0.0f ) {
frustum3 res;

const float an = fovy * 0.5f * (3.141592653589f/180.0f);
const float si = sinf(an);
const float co = cosf(an);
#if 0
mPlanes[0] = vec4(  -co, 0.0f, si*aspect, 0.0f );
mPlanes[1] = vec4(   co, 0.0f, si*aspect, 0.0f );
mPlanes[2] = vec4( 0.0f,   co, si,        0.0f );
mPlanes[3] = vec4( 0.0f,  -co, si,        0.0f );
#else
mPlanes[0] = vec4(  -co, 0.0f, si, 0.0f );
mPlanes[1] = vec4(   co, 0.0f, si, 0.0f );
mPlanes[2] = vec4( 0.0f,   co, si/aspect,        0.0f );
mPlanes[3] = vec4( 0.0f,  -co, si/aspect,        0.0f );
#endif
mPlanes[4] = vec4( 0.0f, 0.0f, -1.0f,     zfar );
mPlanes[5] = vec4( 0.0f, 0.0f,  1.0f,   -znear );

//mPoints[0] = intersectPlanes( mPlanes[1], mPlanes[2], mPlanes[4] ); // same as bellow, just that *zar/near
//mPoints[1] = intersectPlanes( mPlanes[0], mPlanes[2], mPlanes[4] );
//mPoints[2] = intersectPlanes( mPlanes[0], mPlanes[3], mPlanes[4] );
//mPoints[3] = intersectPlanes( mPlanes[1], mPlanes[3], mPlanes[4] );
//mPoints[4] = vec3( -right, -top, near );
//mPoints[5] = vec3(  right, -top, near );
//mPoints[6] = vec3(  right,  top, near );
//mPoints[7] = vec3( -right,  top, near );

for ( Si32 i=0; i<6; i++ )
{
// normalize plane
const float im = inverseLength( mPlanes[i].xyz() );
mPlanes[i] *= im;
// move plane outwards
//mPlane[i].w += expansion;
}
}
*/

// 0: outside  1: inside/intersect
inline Si32 BoxInFrustum(Frustum3F const &fru, Bound3F const &box) {
  float band = 0.0f;

  // check box outside/inside of frustum
  for (Si32 i = 0; i < 6; i++) {
    Si32 out = 0;
    out += ((Dot(fru.planes[i], Vec4F(box.min_x, box.min_y, box.min_z, 1.0f))
      < -band) ? 1 : 0);
    out += ((Dot(fru.planes[i], Vec4F(box.max_x, box.min_y, box.min_z, 1.0f))
      < -band) ? 1 : 0);
    out += ((Dot(fru.planes[i], Vec4F(box.min_x, box.max_y, box.min_z, 1.0f))
      < -band) ? 1 : 0);
    out += ((Dot(fru.planes[i], Vec4F(box.max_x, box.max_y, box.min_z, 1.0f))
      < -band) ? 1 : 0);
    out += ((Dot(fru.planes[i], Vec4F(box.min_x, box.min_y, box.max_z, 1.0f))
      < -band) ? 1 : 0);
    out += ((Dot(fru.planes[i], Vec4F(box.max_x, box.min_y, box.max_z, 1.0f))
      < -band) ? 1 : 0);
    out += ((Dot(fru.planes[i], Vec4F(box.min_x, box.max_y, box.max_z, 1.0f))
      < -band) ? 1 : 0);
    out += ((Dot(fru.planes[i], Vec4F(box.max_x, box.max_y, box.max_z, 1.0f))
      < -band) ? 1 : 0);
    if (out == 8) {
      return 0;
    }
  }

  // check frustum outside/inside box
  Si32 out;
  out = 0;
  for (Si32 i = 0; i < 8; i++) {
    out += ((fru.points[i].x > (box.max_x + band)) ? 1 : 0);
  }
  if (out == 8) {
    return 0;
  }
  out = 0;
  for (Si32 i = 0; i < 8; i++) {
    out += ((fru.points[i].x < (box.min_x - band)) ? 1 : 0);
  }
  if (out == 8) {
    return 0;
  }
  out = 0;
  for (Si32 i = 0; i < 8; i++) {
    out += ((fru.points[i].y > (box.max_y + band)) ? 1 : 0);
  }
  if (out == 8) {
    return 0;
  }
  out = 0;
  for (Si32 i = 0; i < 8; i++) {
    out += ((fru.points[i].y < (box.min_y - band)) ? 1 : 0);
  }
  if (out == 8) {
    return 0;
  }
  out = 0;
  for (Si32 i = 0; i < 8; i++) {
    out += ((fru.points[i].z > (box.max_z + band)) ? 1 : 0);
  }
  if (out == 8) {
    return 0;
  }
  out = 0;
  for (Si32 i = 0; i < 8; i++) {
    out += ((fru.points[i].z < (box.min_z - band)) ? 1 : 0);
  }
  if (out == 8) {
    return 0;
  }

  return 1;
}
/// @}

}  // namespace arctic

#endif  // ENGINE_FRUSTUM3F_H_
