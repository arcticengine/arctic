// Collision and physics: swept sphere vs triangle, the regressions taken from
// Hover Racer logs, CollideSoup, the solver and PhysicsWorld.
#define TEST_NO_MAIN
#include "test_helpers.h"

static CollisionTriangle MakeTri(const Vec3F &a, const Vec3F &b,
    const Vec3F &c) {
  CollisionTriangle tri;
  bool ok = tri.Set(a, b, c);
  TEST_CHECK_(ok, "CollisionTriangle::Set must accept a non-degenerate triangle");
  return tri;
}

static bool HitNear(const SphereTriangleHit &hit, float t, float tol) {
  return hit.hit && fabsf(hit.time - t) <= tol;
}

void test_sphere_vs_triangle_degenerate_and_inside() {
  CollisionTriangle tri;
  TEST_CHECK_(!tri.Set(Vec3F(0.0f, 0.0f, 0.0f), Vec3F(0.0f, 0.0f, 0.0f),
      Vec3F(1.0f, 0.0f, 0.0f)),
      "A triangle with a zero-length edge must be rejected");
  TEST_CHECK_(!tri.Set(Vec3F(0.0f, 0.0f, 0.0f), Vec3F(1.0f, 0.0f, 0.0f),
      Vec3F(2.0f, 0.0f, 0.0f)),
      "A collinear triangle must be rejected");

  tri = MakeTri(Vec3F(-1.0f, 0.0f, -1.0f), Vec3F(1.0f, 0.0f, -1.0f),
      Vec3F(0.0f, 0.0f, 1.0f));
  TEST_CHECK_(tri.ContainsPoint(Vec3F(0.0f, 0.0f, 0.0f)),
      "The centroid projection must be inside the triangle");
  TEST_CHECK_(!tri.ContainsPoint(Vec3F(3.0f, 0.0f, 0.0f)),
      "A point well outside the triangle must be rejected");
  TEST_CHECK_(!tri.ContainsPoint(Vec3F(0.0f, 0.0f, -1.0f)),
      "A point on an edge is treated as outside so the edge test owns it");

  Vec3F closest = tri.ClosestPoint(Vec3F(0.0f, 4.0f, 0.0f));
  TEST_CHECK_(fabsf(closest.x) < 1e-4f && fabsf(closest.y) < 1e-4f &&
      fabsf(closest.z) < 1e-4f,
      "Closest point of (0,4,0) to the XZ triangle must be the origin, got "
      "(%f,%f,%f)", closest.x, closest.y, closest.z);

  Vec3F on_vertex = tri.ClosestPoint(Vec3F(-8.0f, 3.0f, -8.0f));
  TEST_CHECK_(fabsf(on_vertex.x + 1.0f) < 1e-4f &&
      fabsf(on_vertex.z + 1.0f) < 1e-4f,
      "Closest point to a far corner must be vertex A, got (%f,%f,%f)",
      on_vertex.x, on_vertex.y, on_vertex.z);
}

void test_sphere_vs_triangle_static_overlap() {
  CollisionTriangle tri = MakeTri(Vec3F(-2.0f, 0.0f, -2.0f),
      Vec3F(2.0f, 0.0f, -2.0f), Vec3F(0.0f, 0.0f, 2.0f));

  MovingSphere miss(Vec3F(0.0f, 3.0f, 0.0f), 1.0f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SphereOverlapsTriangle(miss, tri),
      "A sphere 3 units above a plane with radius 1 must miss");
  TEST_CHECK_(!SphereTriangleContact(miss, tri).hit,
      "SphereTriangleContact must miss the same far sphere");

  MovingSphere face(Vec3F(0.0f, 0.4f, 0.0f), 1.0f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereOverlapsTriangle(face, tri),
      "A sphere whose center is 0.4 above the face with radius 1 must overlap");
  SphereTriangleHit face_hit = SphereTriangleContact(face, tri);
  TEST_CHECK_(face_hit.hit && fabsf(face_hit.time) < 1e-6f,
      "Static face overlap must report time 0");
  TEST_CHECK_(fabsf(face_hit.point.y) < 1e-4f,
      "Static face contact point must lie on the plane, y=%f", face_hit.point.y);

  MovingSphere edge(Vec3F(0.0f, 0.0f, -2.4f), 0.5f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereOverlapsTriangle(edge, tri),
      "A sphere 0.4 past the AB edge with radius 0.5 must overlap the edge");

  MovingSphere vertex(Vec3F(-2.3f, 0.0f, -2.0f), 0.4f,
      Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereOverlapsTriangle(vertex, tri),
      "A sphere 0.3 past vertex A with radius 0.4 must overlap the vertex");

  MovingSphere far_bound(Vec3F(50.0f, 0.0f, 50.0f), 1.0f,
      Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SphereOverlapsTriangle(far_bound, tri),
      "The bounding sphere must reject a distant query without a hit");
}

void test_sphere_vs_triangle_swept_face() {
  CollisionTriangle tri = MakeTri(Vec3F(-2.0f, 0.0f, -2.0f),
      Vec3F(2.0f, 0.0f, -2.0f), Vec3F(0.0f, 0.0f, 2.0f));

  // Center starts at y=3, radius 1, moves by (0,-4,0). First touch when y=1,
  // so t = (3-1)/4 = 0.5. Leaving the plane would be y=-1 at t=1, and that
  // later root must not be reported.
  MovingSphere toward(Vec3F(0.0f, 3.0f, 0.0f), 1.0f, Vec3F(0.0f, -4.0f, 0.0f));
  SphereTriangleHit hit = SweptSphereVsTriangle(toward, tri);
  TEST_CHECK_(HitNear(hit, 0.5f, 1e-4f),
      "Head-on face hit must be t=0.5, got hit=%d t=%f", hit.hit, hit.time);
  TEST_CHECK_(fabsf(hit.point.y) < 1e-3f,
      "Face contact must lie on the plane, y=%f", hit.point.y);
  TEST_CHECK_(hit.normal.y > 0.0f,
      "Face normal must point toward the incoming sphere, ny=%f", hit.normal.y);

  MovingSphere through(Vec3F(0.0f, 3.0f, 0.0f), 1.0f, Vec3F(0.0f, -6.0f, 0.0f));
  SphereTriangleHit enter = SweptSphereVsTriangle(through, tri);
  TEST_CHECK_(HitNear(enter, 1.0f / 3.0f, 1e-3f),
      "A sphere that would exit the plane inside the interval must still "
      "report the entry time 1/3, got hit=%d t=%f", enter.hit, enter.time);

  MovingSphere too_short(Vec3F(0.0f, 3.0f, 0.0f), 1.0f,
      Vec3F(0.0f, -1.5f, 0.0f));
  SphereTriangleHit miss_short = SweptSphereVsTriangle(too_short, tri);
  TEST_CHECK_(!miss_short.hit,
      "Stopping 0.5 above the plane with radius 1 must miss, t=%f",
      miss_short.time);

  MovingSphere away(Vec3F(0.0f, 3.0f, 0.0f), 1.0f, Vec3F(0.0f, 4.0f, 0.0f));
  TEST_CHECK_(!SweptSphereVsTriangle(away, tri).hit,
      "A sphere moving away from the plane must miss");

  MovingSphere parallel(Vec3F(0.0f, 3.0f, 0.0f), 1.0f,
      Vec3F(4.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SweptSphereVsTriangle(parallel, tri).hit,
      "A sphere moving parallel to the plane and out of the slab must miss");

  // Skin on the plane, moving along it: must slide, not freeze as t=0 overlap.
  MovingSphere rest_slide(Vec3F(0.0f, 1.0f, 0.0f), 1.0f,
      Vec3F(0.5f, 0.0f, 0.0f));
  SphereTriangleHit slide = SweptSphereVsTriangle(rest_slide, tri);
  TEST_CHECK_(!slide.hit || slide.time > 1e-4f,
      "A sphere resting on the face and moving parallel must not freeze as "
      "t=0 overlap, got hit=%d t=%f", slide.hit, slide.time);

  MovingSphere from_below(Vec3F(0.0f, -3.0f, 0.0f), 1.0f,
      Vec3F(0.0f, 4.0f, 0.0f));
  SphereTriangleHit back = SweptSphereVsTriangle(from_below, tri);
  TEST_CHECK_(HitNear(back, 0.5f, 1e-4f),
      "A hit from the back of the plane must still be found at t=0.5, got "
      "hit=%d t=%f", back.hit, back.time);
}

void test_sphere_vs_triangle_swept_edge_vertex_and_overlap() {
  CollisionTriangle tri = MakeTri(Vec3F(0.0f, 0.0f, 0.0f),
      Vec3F(4.0f, 0.0f, 0.0f), Vec3F(0.0f, 4.0f, 0.0f));

  // Sphere sits in the triangle's plane, so the face test cannot fire, and
  // the AB edge (y=0, z=0) is the first feature. Center y=-3, radius 1,
  // velocity (0,3,0): contact when y=-1, t=2/3.
  MovingSphere to_edge(Vec3F(2.0f, -3.0f, 0.0f), 1.0f, Vec3F(0.0f, 3.0f, 0.0f));
  SphereTriangleHit edge = SweptSphereVsTriangle(to_edge, tri);
  TEST_CHECK_(HitNear(edge, 2.0f / 3.0f, 1e-3f),
      "Edge hit must be t=2/3, got hit=%d t=%f", edge.hit, edge.time);
  TEST_CHECK_(fabsf(edge.point.x - 2.0f) < 1e-3f &&
      fabsf(edge.point.y) < 1e-3f,
      "Edge contact must sit on AB at x=2, got (%f,%f,%f)",
      edge.point.x, edge.point.y, edge.point.z);

  // Vertex A at the origin. Start at (-3,-3,0), radius 1, velocity (3,3,0).
  // |start| = 3*sqrt(2), contact when remaining distance is 1:
  // |t-1| * 3 * sqrt(2) = 1 => t = 1 - 1/(3*sqrt(2)).
  float expect_v = 1.0f - 1.0f / (3.0f * std::sqrt(2.0f));
  MovingSphere to_vertex(Vec3F(-3.0f, -3.0f, 0.0f), 1.0f,
      Vec3F(3.0f, 3.0f, 0.0f));
  SphereTriangleHit vertex = SweptSphereVsTriangle(to_vertex, tri);
  TEST_CHECK_(HitNear(vertex, expect_v, 2e-3f),
      "Vertex hit must be t=%f, got hit=%d t=%f", expect_v, vertex.hit,
      vertex.time);
  TEST_CHECK_(fabsf(vertex.point.x) < 1e-3f && fabsf(vertex.point.y) < 1e-3f,
      "Vertex contact must be at A, got (%f,%f,%f)",
      vertex.point.x, vertex.point.y, vertex.point.z);

  MovingSphere already(Vec3F(1.0f, 0.4f, 0.0f), 1.0f,
      Vec3F(0.0f, 2.0f, 0.0f));
  SphereTriangleHit overlap = SweptSphereVsTriangle(already, tri);
  TEST_CHECK_(overlap.hit && fabsf(overlap.time) < 1e-6f,
      "A sphere that already intersects the triangle must report t=0, got "
      "hit=%d t=%f", overlap.hit, overlap.time);

  MovingSphere still(Vec3F(1.0f, 0.4f, 0.0f), 1.0f, Vec3F(0.0f, 0.0f, 0.0f));
  SphereTriangleHit rest = SweptSphereVsTriangle(still, tri);
  TEST_CHECK_(rest.hit && fabsf(rest.time) < 1e-6f,
      "A resting overlapping sphere (zero velocity) must still report t=0");
  MovingSphere rest_miss(Vec3F(0.0f, 8.0f, 0.0f), 1.0f,
      Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SweptSphereVsTriangle(rest_miss, tri).hit,
      "A resting sphere that does not overlap must miss");
}

void test_sphere_vs_triangles_earliest_and_empty() {
  CollisionTriangle near_t = MakeTri(Vec3F(-1.0f, 1.0f, -1.0f),
      Vec3F(1.0f, 1.0f, -1.0f), Vec3F(0.0f, 1.0f, 1.0f));
  CollisionTriangle far_t = MakeTri(Vec3F(-1.0f, 0.0f, -1.0f),
      Vec3F(1.0f, 0.0f, -1.0f), Vec3F(0.0f, 0.0f, 1.0f));
  CollisionTriangle tris[2] = {far_t, near_t};

  // From y=3, radius 1, velocity (0,-3,0). Near plane y=1 is touched at t=1/3,
  // far plane y=0 at t=2/3. The far triangle is listed first so a loop that
  // forgets to keep the earliest hit would return the wrong one.
  MovingSphere sph(Vec3F(0.0f, 3.0f, 0.0f), 1.0f, Vec3F(0.0f, -3.0f, 0.0f));
  SphereTriangleHit hit = SweptSphereVsTriangles(sph, tris, 2);
  TEST_CHECK_(HitNear(hit, 1.0f / 3.0f, 1e-3f),
      "The earlier of two face hits must win, got hit=%d t=%f",
      hit.hit, hit.time);
  TEST_CHECK_(fabsf(hit.point.y - 1.0f) < 1e-3f,
      "The surviving contact must be on the near plane y=1, y=%f",
      hit.point.y);

  TEST_CHECK_(!SweptSphereVsTriangles(sph, tris, 0).hit,
      "A zero-count list must miss");
  TEST_CHECK_(!SweptSphereVsTriangles(sph, nullptr, 2).hit,
      "A null triangle list must miss");

  // Camera-style query: look-at at the origin, desired camera behind a wall
  // at z=-2. Sphere radius 0.5, offset (0,0,-8). Contact when the sphere
  // first reaches z=-2, i.e. center at z=-1.5, t = 1.5/8.
  CollisionTriangle wall = MakeTri(Vec3F(-5.0f, -5.0f, -2.0f),
      Vec3F(5.0f, -5.0f, -2.0f), Vec3F(0.0f, 5.0f, -2.0f));
  MovingSphere cam(Vec3F(0.0f, 0.0f, 0.0f), 0.5f, Vec3F(0.0f, 0.0f, -8.0f));
  SphereTriangleHit cam_hit = SweptSphereVsTriangle(cam, wall);
  TEST_CHECK_(HitNear(cam_hit, 1.5f / 8.0f, 1e-3f),
      "Camera boom into a wall must stop at t=1.5/8, got hit=%d t=%f",
      cam_hit.hit, cam_hit.time);
  Vec3F cam_pos = cam.center + cam.velocity * cam_hit.time;
  TEST_CHECK_(fabsf(cam_pos.z + 1.5f) < 1e-3f,
      "The camera sphere center must sit 0.5 in front of the wall, z=%f",
      cam_pos.z);
}

void test_line_segment_pierces_triangle() {
  CollisionTriangle tri = MakeTri(Vec3F(-2.0f, 0.0f, -2.0f),
      Vec3F(2.0f, 0.0f, -2.0f), Vec3F(0.0f, 0.0f, 2.0f));

  float t = -1.0f;
  Vec3F p;
  TEST_CHECK_(LineSegmentPiercesTriangle(Vec3F(0.0f, 1.0f, 0.0f),
      Vec3F(0.0f, -1.0f, 0.0f), tri, &t, &p),
      "A segment from above the face to below it through the centroid must "
      "pierce");
  TEST_CHECK_(fabsf(t - 0.5f) < 1e-4f && fabsf(p.y) < 1e-4f,
      "The pierce must be at t=0.5 on the plane, got t=%f p=(%f,%f,%f)",
      t, p.x, p.y, p.z);

  TEST_CHECK_(!LineSegmentPiercesTriangle(Vec3F(8.0f, 1.0f, 0.0f),
      Vec3F(8.0f, -1.0f, 0.0f), tri, nullptr, nullptr),
      "A segment that crosses the plane outside the triangle must not pierce");

  TEST_CHECK_(!LineSegmentPiercesTriangle(Vec3F(0.0f, 2.0f, 0.0f),
      Vec3F(0.0f, 1.0f, 0.0f), tri, nullptr, nullptr),
      "Both endpoints above the plane must not pierce");

  TEST_CHECK_(!LineSegmentPiercesTriangle(Vec3F(0.0f, 1.0f, 0.0f),
      Vec3F(0.0f, 0.1f, 0.0f), tri, nullptr, nullptr),
      "Stopping short of the plane must not pierce");

  TEST_CHECK_(LineSegmentPiercesTriangle(Vec3F(0.0f, -1.0f, 0.0f),
      Vec3F(0.0f, 1.0f, 0.0f), tri, &t, nullptr),
      "A pierce from below must still count: the center changed sides");
}

void test_swept_sphere_resting_face_does_not_fall_through() {
  CollisionTriangle tri = MakeTri(Vec3F(-2.0f, 0.0f, -2.0f),
      Vec3F(2.0f, 0.0f, -2.0f), Vec3F(0.0f, 0.0f, 2.0f));

  // Skin on the plane, moving into it. The 0.997-radius "not buried" exception
  // used to skip this so a parallel slide could continue; a vertical drop
  // then missed the face (in-slab, edges far away) and fell through.
  MovingSphere into(Vec3F(0.0f, 1.0f, 0.0f), 1.0f, Vec3F(0.0f, -2.0f, 0.0f));
  SphereTriangleHit hit = SweptSphereVsTriangle(into, tri);
  TEST_CHECK_(hit.hit && hit.time < 1e-3f,
      "A sphere resting on a face and moving into it must hit at t=0, got "
      "hit=%d t=%f", hit.hit, hit.time);

  MovingSphere slide(Vec3F(0.0f, 1.0f, 0.0f), 1.0f, Vec3F(0.5f, 0.0f, 0.0f));
  SphereTriangleHit along = SweptSphereVsTriangle(slide, tri);
  TEST_CHECK_(!along.hit || along.time > 1e-4f,
      "A sphere resting on the face and moving parallel must still slide, "
      "got hit=%d t=%f", along.hit, along.time);
}

void test_slope_into_plane_slides_along_tangent() {
  // Plane y = x, outward n = (-1, 1, 0)/sqrt(2). A sphere sitting on it
  // with a horizontal move has an into-plane component, so the sweep
  // freezes at t=0. The remainder after removing that component must stay
  // in the plane (along the slope), not hop along the normal.
  CollisionTriangle slope = MakeTri(Vec3F(0.0f, 0.0f, -2.0f),
      Vec3F(0.0f, 0.0f, 2.0f), Vec3F(4.0f, 4.0f, -2.0f));
  Vec3F n = Normalize(Vec3F(-1.0f, 1.0f, 0.0f));
  Vec3F start(1.0f, 1.0f, 0.0f);
  start = start + n * 1.0f;
  MovingSphere sph(start, 1.0f, Vec3F(1.0f, 0.0f, 0.0f));
  SphereTriangleHit hit = SweptSphereVsTriangle(sph, slope);
  TEST_CHECK_(hit.hit && hit.time < 1e-3f,
      "A horizontal move into a slope the sphere is sitting on must hit at "
      "t=0, got hit=%d t=%f", hit.hit, hit.time);

  Vec3F remain = sph.velocity;
  float vn = Dot(n, remain);
  TEST_CHECK_(vn < 0.0f, "The horizontal move must point into the slope, vn=%f",
      vn);
  remain = remain - n * vn;
  TEST_CHECK_(fabsf(Dot(n, remain)) < 1e-5f,
      "The clipped remainder must lie in the plane, n·remain=%f",
      Dot(n, remain));

  // Downhill at constant Y leaves the plane; a stick-down like the craft
  // uses makes the combined move hit, and the remainder goes down the slope.
  Vec3F down(-1.0f, -1.8f, 0.0f);
  MovingSphere fall(start, 1.0f, down);
  SphereTriangleHit hit_down = SweptSphereVsTriangle(fall, slope);
  TEST_CHECK_(hit_down.hit && hit_down.time < 1e-3f,
      "A downhill move with stick-to-ground must hit the slope at t=0, got "
      "hit=%d t=%f", hit_down.hit, hit_down.time);
  float vn_down = Dot(n, down);
  Vec3F along = down - n * vn_down;
  TEST_CHECK_(fabsf(Dot(n, along)) < 1e-4f,
      "Downhill remainder must stay in the plane, n·along=%f", Dot(n, along));
  TEST_CHECK_(along.y < -1e-4f && along.x < -1e-4f,
      "Downhill remainder must go down the slope, along=(%f,%f,%f)",
      along.x, along.y, along.z);
}

void test_hover_racer_log_drop_through_rock_slope() {
  // Exact numbers from a SPHERE TUNNEL log: the ride sphere sat on a rock
  // face (signed distance ~ radius) then DropRideSphere moved (0, -2.04, 0)
  // and the center crossed that face, landing on the world-box floor.
  const float radius = 0.276000023f;
  Vec3F start(21.3660736f, 7.0842123f, 68.9416199f);
  Vec3F vel(0.0f, -2.03896618f, 0.0f);
  CollisionTriangle slope = MakeTri(
      Vec3F(19.312149f, 4.1875186f, 72.4813919f),
      Vec3F(22.6897907f, 6.95716333f, 71.8824158f),
      Vec3F(21.4869881f, 7.48888588f, 66.4704208f));

  float pierce_t = -1.0f;
  TEST_CHECK_(LineSegmentPiercesTriangle(start, start + vel, slope, &pierce_t,
      nullptr),
      "The logged drop must pierce the rock face, so the test is the real miss");
  TEST_CHECK_(fabsf(pierce_t - 0.174380094f) < 1e-4f,
      "Pierce time must match the log (0.17438), got %f", pierce_t);

  float d0 = slope.SignedDistance(start);
  TEST_CHECK_(d0 > 0.0f && d0 < radius,
      "The sphere must start on the outside of the face, d0=%f radius=%f",
      d0, radius);

  MovingSphere sph(start, radius, vel);
  SphereTriangleHit hit = SweptSphereVsTriangle(sph, slope);
  TEST_CHECK_(hit.hit,
      "Dropping a sphere that is already sitting on the rock face must hit "
      "that face, got hit=%d t=%f", hit.hit, hit.time);
  TEST_CHECK_(hit.time < pierce_t,
      "The sphere skin must hit before the center crosses the plane: t=%f "
      "pierce=%f", hit.time, pierce_t);

  CollisionTriangle soup[9];
  soup[0] = MakeTri(Vec3F(199.999985f, 4.54269409f, 199.999985f),
      Vec3F(199.999985f, 4.54269409f, -199.999985f),
      Vec3F(-200.0f, 4.54269409f, -199.999985f));
  soup[1] = MakeTri(Vec3F(199.999985f, 4.54269409f, 199.999985f),
      Vec3F(-200.0f, 4.54269409f, -199.999985f),
      Vec3F(-200.0f, 4.54269409f, 199.999985f));
  soup[2] = MakeTri(Vec3F(199.999985f, 4.50699615f, -199.999985f),
      Vec3F(199.999985f, 4.50699615f, 199.999985f),
      Vec3F(-200.0f, 4.50699615f, 199.999985f));
  soup[3] = MakeTri(Vec3F(199.999985f, 4.50699615f, -199.999985f),
      Vec3F(-200.0f, 4.50699615f, 199.999985f),
      Vec3F(-200.0f, 4.50699615f, -199.999985f));
  soup[4] = MakeTri(Vec3F(22.6897907f, 6.95716333f, 71.8824158f),
      Vec3F(22.9879951f, 7.25306082f, 66.0175858f),
      Vec3F(21.4869881f, 7.48888588f, 66.4704208f));
  soup[5] = slope;
  soup[6] = MakeTri(Vec3F(19.312149f, 4.1875186f, 72.4813919f),
      Vec3F(21.4869881f, 7.48888588f, 66.4704208f),
      Vec3F(17.1056366f, 4.1875186f, 67.3578491f));
  soup[7] = MakeTri(Vec3F(19.312149f, 4.1875186f, 72.4813919f),
      Vec3F(17.1056366f, 4.1875186f, 67.3578491f),
      Vec3F(24.761076f, 4.19974661f, 65.482666f));
  soup[8] = MakeTri(Vec3F(19.312149f, 4.1875186f, 72.4813919f),
      Vec3F(24.761076f, 4.19974661f, 65.482666f),
      Vec3F(26.5213814f, 4.19974661f, 71.4986954f));

  SphereTriangleHit soup_hit = SweptSphereVsTriangles(sph, soup, 9);
  TEST_CHECK_(soup_hit.hit && soup_hit.time < pierce_t,
      "Among the logged candidates the rock face must win before the center "
      "crosses it, got hit=%d t=%f", soup_hit.hit, soup_hit.time);
  Vec3F stopped = sph.center + sph.velocity * (soup_hit.time * 0.9f);
  TEST_CHECK_(slope.SignedDistance(stopped) > 0.0f,
      "StopAtHit along the drop must leave the center on the outside of the "
      "rock, d=%f", slope.SignedDistance(stopped));
}

void test_hover_racer_log_zero_vel_drop_through_slope() {
  // Later tunnel: SeparateField resolved with wish_vel=0. The sphere was
  // sitting on a rock (d0 == radius). Orthogonal closest-point sat on an
  // edge beyond the radius, so static contact missed; a long DropRideSphere
  // then crossed the face.
  const float radius = 0.276000023f;
  Vec3F start(4.85254049f, 6.21253967f, 37.0080338f);
  Vec3F drop_vel(0.0f, -1.25446129f, 0.0f);
  CollisionTriangle slope = MakeTri(
      Vec3F(2.69449186f, 5.81134081f, 34.9970665f),
      Vec3F(6.99815607f, 6.21761894f, 39.8411827f),
      Vec3F(8.5653286f, 5.79255247f, 38.8543282f));

  float d0 = slope.SignedDistance(start);
  TEST_CHECK_(d0 > 0.0f && d0 <= radius * 1.002f,
      "The logged start must sit on the outside of the face, d0=%f radius=%f",
      d0, radius);

  Vec3F q = slope.ClosestPoint(start);
  float closest_d = Length(start - q);
  TEST_CHECK_(closest_d > radius,
      "This log is the edge-closest miss: dist to triangle %f must exceed "
      "radius %f so static overlap is not a stand-in for the fix",
      closest_d, radius);

  Vec3F below = start;
  below.y -= radius * 3.0f;
  float support_t = -1.0f;
  TEST_CHECK_(LineSegmentPiercesTriangle(start, below, slope, &support_t,
      nullptr),
      "A vertical sit probe long enough to cross the plane must pierce");

  MovingSphere probe(start, radius, below - start);
  SphereTriangleHit probe_hit = SweptSphereVsTriangle(probe, slope);
  TEST_CHECK_(probe_hit.hit && probe_hit.time < 1e-3f,
      "A downward probe that would carry the center through the sitting face "
      "must hit at t=0, got hit=%d t=%f", probe_hit.hit, probe_hit.time);

  float pierce_t = -1.0f;
  TEST_CHECK_(LineSegmentPiercesTriangle(start, start + drop_vel, slope,
      &pierce_t, nullptr),
      "The logged drop must pierce the rock face");
  MovingSphere drop(start, radius, drop_vel);
  SphereTriangleHit drop_hit = SweptSphereVsTriangle(drop, slope);
  TEST_CHECK_(drop_hit.hit && drop_hit.time < pierce_t,
      "The long drop must hit the sitting face before the center crosses it, "
      "got hit=%d t=%f pierce=%f", drop_hit.hit, drop_hit.time, pierce_t);

  // Height queries start a little above the craft. That start is outside
  // the slab, so a pierce check that only fired in-slab missed, the query
  // returned the start as "ground", and ClampHover tossed the craft up.
  Vec3F high = start;
  high.y += 1.0f;
  Vec3F high_vel(0.0f, -2.5f, 0.0f);
  float high_pierce = -1.0f;
  TEST_CHECK_(LineSegmentPiercesTriangle(high, high + high_vel, slope,
      &high_pierce, nullptr),
      "A height-query drop from above the sitting pose must still pierce");
  MovingSphere from_above(high, radius, high_vel);
  SphereTriangleHit above_hit = SweptSphereVsTriangle(from_above, slope);
  TEST_CHECK_(above_hit.hit && above_hit.time > 1e-4f &&
      above_hit.time < high_pierce,
      "A drop from above the tilted face must hit when the skin meets the "
      "plane, not only after the center is already in the slab, got hit=%d "
      "t=%f pierce=%f", above_hit.hit, above_hit.time, high_pierce);
}

void test_hover_racer_log_stall_on_shallow_slope() {
  // SPHERE STALL: sitting on a nearly-flat face (ny~0.995). A horizontal
  // wish has a real into-plane component, so the first sweep freezes at
  // t=0. The clipped remainder is in the plane up to float noise; that
  // noise used to count as into-plane and freeze the slide too.
  const float radius = 0.276000023f;
  Vec3F start(22.4530811f, 6.48346138f, 91.1421432f);
  CollisionTriangle slope = MakeTri(
      Vec3F(23.2624588f, 6.34688902f, 86.4868851f),
      Vec3F(21.1819f, 6.34688902f, 90.5246277f),
      Vec3F(22.7512684f, 6.16313601f, 91.507988f));
  Vec3F wish(-0.00658798218f, 5.62667847e-05f, 0.00238037109f);

  MovingSphere sitting(start, radius, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereTriangleContact(sitting, slope).hit,
      "The logged stall start must be sitting on the face");

  MovingSphere into(start, radius, wish);
  SphereTriangleHit hit = SweptSphereVsTriangle(into, slope);
  TEST_CHECK_(hit.hit && hit.time < 1e-3f,
      "A horizontal wish into the shallow slope must still freeze at t=0, "
      "got hit=%d t=%f", hit.hit, hit.time);

  Vec3F n = slope.n;
  float nl = Length(n);
  TEST_CHECK_(nl > 1e-8f, "The stall face must have a normal");
  n = n / nl;
  if (n.y < 0.0f) {
    n = n * -1.0f;
  }
  Vec3F remain = wish - n * Dot(n, wish);
  TEST_CHECK_(fabsf(Dot(n, remain)) < 1e-6f,
      "Clipped remainder must lie in the plane, n·remain=%f", Dot(n, remain));
  TEST_CHECK_(Length(remain) > 0.5f * Length(wish),
      "Most of the wish must survive as along-slope slide, |remain|=%f "
      "|wish|=%f", Length(remain), Length(wish));

  MovingSphere along(start, radius, remain);
  SphereTriangleHit slide = SweptSphereVsTriangle(along, slope);
  TEST_CHECK_(!slide.hit || slide.time > 1e-4f,
      "The in-plane remainder must slide, not freeze as t=0, got hit=%d "
      "t=%f", slide.hit, slide.time);

  CollisionTriangle floor = MakeTri(Vec3F(-2.0f, 0.0f, -2.0f),
      Vec3F(2.0f, 0.0f, -2.0f), Vec3F(0.0f, 0.0f, 2.0f));
  MovingSphere noisy(Vec3F(0.0f, 1.0f, 0.0f), 1.0f,
      Vec3F(0.5f, -5.0e-6f, 0.0f));
  SphereTriangleHit noisy_hit = SweptSphereVsTriangle(noisy, floor);
  TEST_CHECK_(!noisy_hit.hit || noisy_hit.time > 1e-4f,
      "A 1e-5 relative into-plane leak on a parallel slide must not freeze, "
      "got hit=%d t=%f", noisy_hit.hit, noisy_hit.time);
}

static Vec3F SitContactN(const Vec3F &center, const CollisionTriangle &tri) {
  Vec3F q = tri.ClosestPoint(center);
  Vec3F cn = center - q;
  if (cn.y >= 0.0f && LengthSquared(cn) > 1e-16f) {
    return cn / Length(cn);
  }
  Vec3F n = tri.n;
  float nl = Length(n);
  TEST_CHECK_(nl > 1e-8f, "A sitting triangle must have a normal");
  n = n / nl;
  if (n.y < 0.0f) {
    n = n * -1.0f;
  }
  return n;
}

static Vec3F ClipIntoPlanes(Vec3F vel, const Vec3F *ns, Si32 count) {
  for (Si32 it = 0; it < 8; ++it) {
    bool any = false;
    for (Si32 i = 0; i < count; ++i) {
      float vn = Dot(ns[i], vel);
      if (vn < 0.0f) {
        vel = vel - ns[i] * vn;
        any = true;
      }
    }
    if (!any) {
      break;
    }
  }
  return vel;
}

void test_hover_racer_log_stall_on_slope_crease() {
  // SPHERE STALL at a crease: sitting on a nearly-flat floor (15556) and a
  // steeper neighbor (17349). Clipping only the flattest face left the wish
  // pointing into 17349, so the sweep froze at t=0 and one slide then died
  // on the shared edge.
  const float radius = 0.276000023f;
  Vec3F start(8.64257717f, 6.59866142f, 95.0418701f);
  Vec3F wish(-0.00314235687f, -6.10351562e-05f, 0.00205230713f);
  CollisionTriangle flat = MakeTri(
      Vec3F(4.86948061f, 6.40157127f, 93.6529465f),
      Vec3F(13.1847143f, 6.34688902f, 100.295555f),
      Vec3F(10.5962973f, 6.40157127f, 96.0796432f));
  CollisionTriangle slope = MakeTri(
      Vec3F(4.40158939f, 4.1875186f, 90.7114258f),
      Vec3F(4.86948061f, 6.40157127f, 93.6529465f),
      Vec3F(10.5962973f, 6.40157127f, 96.0796432f));
  CollisionTriangle soup[2];
  soup[0] = flat;
  soup[1] = slope;

  MovingSphere sitting(start, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereTriangleContact(sitting, flat).hit,
      "The logged stall start must be sitting on the flat floor");
  TEST_CHECK_(SphereTriangleContact(sitting, slope).hit,
      "The logged stall start must also be sitting on the steep neighbor");

  MovingSphere into(start, radius, wish);
  SphereTriangleHit raw_slope = SweptSphereVsTriangle(into, slope);
  TEST_CHECK_(raw_slope.hit && raw_slope.time < 1e-3f,
      "A raw wish into the steep neighbor must freeze at t=0, got hit=%d "
      "t=%f", raw_slope.hit, raw_slope.time);

  Vec3F flat_face = flat.n / Length(flat.n);
  if (flat_face.y < 0.0f) {
    flat_face = flat_face * -1.0f;
  }
  Vec3F only_flat[1] = {flat_face};
  Vec3F clipped_flat = ClipIntoPlanes(wish, only_flat, 1);
  MovingSphere still_into(start, radius, clipped_flat);
  SphereTriangleHit still = SweptSphereVsTriangle(still_into, slope);
  TEST_CHECK_(still.hit && still.time < 1e-3f,
      "Clipping only the flattest face must still freeze on the neighbor, "
      "got hit=%d t=%f", still.hit, still.time);

  Vec3F c_flat = SitContactN(start, flat);
  Vec3F c_slope = SitContactN(start, slope);
  TEST_CHECK_(c_flat.y >= 0.0f && c_slope.y >= 0.0f,
      "Sitting contact normals must not push the center down");
  Vec3F both[2] = {c_flat, c_slope};
  Vec3F clipped = ClipIntoPlanes(wish, both, 2);
  float wish_xz = sqrtf(wish.x * wish.x + wish.z * wish.z);
  float got_xz = sqrtf(clipped.x * clipped.x + clipped.z * clipped.z);
  TEST_CHECK_(got_xz > 0.5f * wish_xz,
      "Clipping every sitting contact must keep most of the XZ wish, "
      "got_xz=%f wish_xz=%f", got_xz, wish_xz);

  MovingSphere along(start, radius, clipped);
  SphereTriangleHit first = SweptSphereVsTriangles(along, soup, 2);
  TEST_CHECK_(!first.hit || first.time > 1e-4f,
      "The remainder clipped against both contacts must not freeze at t=0, "
      "got hit=%d t=%f", first.hit, first.time);

  Vec3F pos = start;
  Vec3F remain = clipped;
  Vec3F clip_ns[8];
  clip_ns[0] = c_flat;
  clip_ns[1] = c_slope;
  Si32 nclip = 2;
  for (Si32 depth = 0; depth < 5; ++depth) {
    if (LengthSquared(remain) <= 1e-12f) {
      break;
    }
    MovingSphere sph(pos, radius, remain);
    SphereTriangleHit cur = SweptSphereVsTriangles(sph, soup, 2);
    if (!cur.hit || cur.time >= 1.0f) {
      pos = pos + remain;
      break;
    }
    pos = pos + remain * (cur.time * 0.9f);
    float time_left = 1.0f - cur.time;
    if (time_left <= 0.15f) {
      break;
    }
    remain = remain * time_left;
    if (nclip < 8) {
      Vec3F n = cur.normal;
      float nl = Length(n);
      clip_ns[nclip] = (nl > 1e-8f) ? n / nl : Vec3F(0.0f, 1.0f, 0.0f);
      ++nclip;
    }
    remain = ClipIntoPlanes(remain, clip_ns, nclip);
  }
  Vec3F moved = pos - start;
  float moved_xz = sqrtf(moved.x * moved.x + moved.z * moved.z);
  TEST_CHECK_(moved_xz > 0.5f * wish_xz,
      "Sweep-and-slide along the crease must keep most of the XZ wish, "
      "moved_xz=%f wish_xz=%f", moved_xz, wish_xz);
}

void test_hover_racer_log_stall_on_shared_edge() {
  // SPHERE STALL: sitting on a slope and a flat floor that share an edge.
  // Closest points are both on that edge, so clipping only the contact
  // radial left the wish pointing into the slope face and froze at t=0.
  const float radius = 0.276000023f;
  Vec3F start(-0.895374835f, 6.63975525f, 95.5743103f);
  Vec3F wish(-0.00185674429f, -4.76837158e-05f, 0.00234985352f);
  CollisionTriangle slope = MakeTri(
      Vec3F(-5.26638556f, 4.1875186f, 92.0922928f),
      Vec3F(-4.99746513f, 6.40157127f, 95.6680527f),
      Vec3F(-0.61086607f, 6.40157127f, 95.7165451f));
  CollisionTriangle flat = MakeTri(
      Vec3F(-4.99746513f, 6.40157127f, 95.6680527f),
      Vec3F(-0.0349920429f, 6.34688902f, 97.5166397f),
      Vec3F(-0.61086607f, 6.40157127f, 95.7165451f));
  if (slope.n.y < 0.0f) {
    slope = MakeTri(slope.a, slope.c, slope.b);
  }
  if (flat.n.y < 0.0f) {
    flat = MakeTri(flat.a, flat.c, flat.b);
  }
  CollisionTriangle soup[2];
  soup[0] = flat;
  soup[1] = slope;

  MovingSphere sitting(start, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereTriangleContact(sitting, flat).hit,
      "The logged stall start must be sitting on the flat floor");
  TEST_CHECK_(SphereTriangleContact(sitting, slope).hit,
      "The logged stall start must be sitting on the slope");

  Vec3F c_flat = SitContactN(start, flat);
  Vec3F c_slope = SitContactN(start, slope);
  Vec3F only_edge[2] = {c_flat, c_slope};
  Vec3F clipped_edge = ClipIntoPlanes(wish, only_edge, 2);
  MovingSphere still_into(start, radius, clipped_edge);
  SphereTriangleHit still = SweptSphereVsTriangle(still_into, slope);
  TEST_CHECK_(still.hit && still.time < 1e-3f,
      "Clipping only the shared-edge radials must still freeze on the "
      "slope face, got hit=%d t=%f", still.hit, still.time);

  Vec3F n_flat = flat.n / Length(flat.n);
  Vec3F n_slope = slope.n / Length(slope.n);
  Vec3F both_faces[4] = {n_flat, n_slope, c_flat, c_slope};
  Vec3F clipped = ClipIntoPlanes(wish, both_faces, 4);
  float wish_xz = sqrtf(wish.x * wish.x + wish.z * wish.z);
  float got_xz = sqrtf(clipped.x * clipped.x + clipped.z * clipped.z);
  TEST_CHECK_(got_xz > 0.3f * wish_xz,
      "Clipping both face normals must keep along-edge XZ, got_xz=%f "
      "wish_xz=%f", got_xz, wish_xz);

  MovingSphere along(start, radius, clipped);
  SphereTriangleHit first = SweptSphereVsTriangles(along, soup, 2);
  TEST_CHECK_(!first.hit || first.time > 1e-4f,
      "The remainder clipped against both faces must not freeze at t=0, "
      "got hit=%d t=%f", first.hit, first.time);
}

void test_hover_racer_log_slope_hover_spring_fights_sit() {
  // SPHERE STALL on rock 17348: sitting, throttle along +Z, but the hover
  // spring's vertical pull (logged lift about -0.68) is 3D-clipped into the
  // steep face. The remainder runs uphill, XZ freezes, and Y steps down
  // every frame.
  const float radius = 0.276000023f;
  Vec3F start(13.7057219f, 5.5814805f, 91.1003418f);
  Vec3F heading(-0.110253148f, 0.0f, 0.993903518f);
  Vec3F raw(-0.000541687012f, -0.0085849762f, 0.0048828125f);
  CollisionTriangle slope = MakeTri(
      Vec3F(8.33132267f, 4.1875186f, 89.8378601f),
      Vec3F(14.9325733f, 5.83451462f, 91.7753906f),
      Vec3F(13.4836855f, 4.1875186f, 90.2841949f));
  if (slope.n.y < 0.0f) {
    slope = MakeTri(slope.a, slope.c, slope.b);
  }
  Vec3F n = slope.n / Length(slope.n);
  TEST_CHECK_(n.y > 0.45f && n.y < 0.8f,
      "The logged rock must be a steep floor, n.y=%f", n.y);

  MovingSphere sitting(start, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereTriangleContact(sitting, slope).hit,
      "The logged stall start must be sitting on the rock");
  float sd = slope.SignedDistance(start);
  TEST_CHECK_(fabsf(sd - radius) < 0.01f,
      "The sphere must sit on the skin, sd=%f radius=%f", sd, radius);

  float raw_xz = sqrtf(raw.x * raw.x + raw.z * raw.z);
  TEST_CHECK_(fabsf(raw.y) > raw_xz,
      "The logged wish must be dominated by the spring -Y, y=%f xz=%f",
      raw.y, raw_xz);

  Vec3F ns[1] = {n};
  Vec3F clipped = ClipIntoPlanes(raw, ns, 1);
  float wish_along = heading.x * raw.x + heading.z * raw.z;
  float got_along = heading.x * clipped.x + heading.z * clipped.z;
  TEST_CHECK_(wish_along > 0.0f && got_along < 0.0f,
      "Spring -Y clipped into the slope must reverse the throttle, "
      "wish_along=%f got_along=%f", wish_along, got_along);
  TEST_CHECK_(clipped.y < 0.0f,
      "The clipped remainder must still step Y down, y=%f", clipped.y);

  Vec3F throttle = raw;
  throttle.y = 0.0f;
  Vec3F along = ClipIntoPlanes(throttle, ns, 1);
  float keep = heading.x * along.x + heading.z * along.z;
  TEST_CHECK_(keep > 0.3f * wish_along,
      "Throttle without the spring -Y must keep heading, keep=%f wish=%f",
      keep, wish_along);

  MovingSphere go(start, radius, along);
  SphereTriangleHit hit = SweptSphereVsTriangle(go, slope);
  TEST_CHECK_(!hit.hit || hit.time > 1e-4f,
      "The throttle-only remainder must not freeze at t=0, got hit=%d t=%f",
      hit.hit, hit.time);
}

void test_hover_racer_log_still_y_jitter_on_slope() {
  // frame log, standing still at (10.206, y, 95.727): Y climbs to 6.58183
  // then slams to 6.53459 about once a second. speed≈0, lift at the peak
  // is ~0. The hover spring target is gnd+1.6*units; ClampHover min is
  // gnd+1.35*units. The sphere is not sitting, so lift<=0 used to call
  // DropRideSphere and yank Y down to the min, then the spring climbed
  // back.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float kMaxHover = 2.2f;
  Vec3F peak(10.206274f, 6.58182907f, 95.7274475f);
  Vec3F dropped(10.206274f, 6.53458691f, 95.7274475f);
  TEST_CHECK_(fabsf(peak.x - dropped.x) < 1e-5f
          && fabsf(peak.z - dropped.z) < 1e-5f,
      "The logged jitter is vertical: XZ must stay put");
  TEST_CHECK_(peak.y - dropped.y > 0.04f && peak.y - dropped.y < 0.06f,
      "The logged Y slam must be about 0.047, dy=%f", peak.y - dropped.y);

  CollisionTriangle slope = MakeTri(
      Vec3F(4.40158939f, 4.1875186f, 90.7114258f),
      Vec3F(4.86948061f, 6.40157127f, 93.6529465f),
      Vec3F(10.5962973f, 6.40157127f, 96.0796432f));
  if (slope.n.y < 0.0f) {
    slope = MakeTri(slope.a, slope.c, slope.b);
  }
  TEST_CHECK_(slope.n.y / Length(slope.n) > 0.45f,
      "The logged still pose sits above a driveable slope, n.y=%f",
      slope.n.y / Length(slope.n));

  const float length = 0.776229f;
  const float yaw = 8.45870113f;
  Vec3F heading(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F sph_off = heading * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F peak_sph = peak + sph_off;
  MovingSphere at_peak(peak_sph, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SphereTriangleContact(at_peak, slope).hit,
      "At the hover peak the sphere is not sitting, so lift<=0 used to "
      "DropRideSphere");

  Vec3F probe(peak.x, peak.y + 1.2f * units, peak.z);
  float drop_len = probe.y - (peak.y - 2.0f);
  MovingSphere down(probe, radius, Vec3F(0.0f, -drop_len, 0.0f));
  SphereTriangleHit gnd_hit = SweptSphereVsTriangle(down, slope);
  TEST_CHECK_(gnd_hit.hit && gnd_hit.time < 1.0f,
      "A vertical height sample at the logged XZ must hit the slope");
  float t = gnd_hit.time * 0.9f;
  if (t < 0.0f) {
    t = 0.0f;
  }
  float gnd = (probe.y - drop_len * t) - radius;
  float hover = gnd + kHoverHeight * units;
  float min_y = gnd + 1.35f * units;
  float ceiling = gnd + kMaxHover * units;
  TEST_CHECK_(fabsf(hover - peak.y) < 0.03f,
      "The logged peak must be the hover-spring target, hover=%f peak=%f",
      hover, peak.y);
  TEST_CHECK_(fabsf(min_y - dropped.y) < 0.03f,
      "The logged slam must land on ClampHover min, min=%f dropped=%f",
      min_y, dropped.y);
  TEST_CHECK_(peak.y >= min_y && peak.y <= ceiling,
      "The still peak is inside the hover band, min=%f peak=%f ceil=%f",
      min_y, peak.y, ceiling);
  TEST_CHECK_(dropped.y >= min_y - 0.01f && dropped.y <= ceiling,
      "The still slam is also inside the hover band, min=%f dropped=%f",
      min_y, dropped.y);

  // Old rule: !supported && lift<=0 -> DropRideSphere. That is true here
  // and is what slammed Y. A pose already in the hover band must not drop.
  bool same = (peak.y - gnd) < kMaxHover * units * 3.0f;
  bool in_band = same && peak.y >= min_y && peak.y <= ceiling;
  TEST_CHECK_(in_band,
      "The logged still pose is in the hover band and must not be treated "
      "as a fall");
}

// Vertical height sample used by hover_racer QueryHeightBelow / ClampHover:
// drop a sphere, stop at 0.9 * t, report center.y - radius. Only floors.
static float HeightBelowFloors(float x, float z, float from_y, float radius,
    const CollisionTriangle *tris, Si32 count) {
  float dest = from_y - 4.0f;
  float drop = from_y - dest;
  if (drop <= 1e-4f) {
    return from_y - radius;
  }
  MovingSphere sph(Vec3F(x, from_y, z), radius, Vec3F(0.0f, -drop, 0.0f));
  SphereTriangleHit best;
  for (Si32 i = 0; i < count; ++i) {
    if (tris[i].n.y < 0.45f) {
      continue;
    }
    SphereTriangleHit hit = SweptSphereVsTriangle(sph, tris[i]);
    if (hit.hit && hit.time < 1.0f && (!best.hit || hit.time < best.time)) {
      best = hit;
    }
  }
  if (!best.hit || best.time >= 1.0f) {
    return dest - radius;
  }
  float t = best.time * 0.9f;
  if (t < 0.0f) {
    t = 0.0f;
  }
  return (from_y - drop * t) - radius;
}

void test_hover_racer_log_tiny_reverse_y_drop() {
  // New-session log t=25.775: inching backward at ~0.045, Y climbs with
  // lift>0 to 5.35974 then slams to 5.28226 in one frame (dxz=0.00055).
  // Repeats every ~4s at the same crease. The ride sphere sits ahead of
  // the craft on rock 17348; a vertical sample at the craft XZ can switch
  // to the shared-edge neighbor or the dirt and ClampHover / DropRideSphere
  // then yanks Y down to that sample's ceiling.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float kMaxHover = 2.2f;
  const float length = 0.776229f;
  const float yaw = 1.99329352f;
  Vec3F peak(11.4969978f, 5.35974407f, 90.7669067f);
  Vec3F dropped(11.4964914f, 5.2822566f, 90.7671356f);
  TEST_CHECK_(peak.y - dropped.y > 0.07f && peak.y - dropped.y < 0.09f,
      "The logged slam must be about 0.078, dy=%f", peak.y - dropped.y);
  float dxz = sqrtf((dropped.x - peak.x) * (dropped.x - peak.x)
      + (dropped.z - peak.z) * (dropped.z - peak.z));
  TEST_CHECK_(dxz < 0.001f,
      "The logged slam is a tiny reverse, dxz=%f", dxz);

  CollisionTriangle rock = MakeTri(
      Vec3F(8.33132267f, 4.1875186f, 89.8378601f),
      Vec3F(14.9325733f, 5.83451462f, 91.7753906f),
      Vec3F(13.4836855f, 4.1875186f, 90.2841949f));
  CollisionTriangle neighbor = MakeTri(
      Vec3F(8.33132267f, 4.1875186f, 89.8378601f),
      Vec3F(10.5962973f, 6.40157127f, 96.0796432f),
      Vec3F(14.9325733f, 5.83451462f, 91.7753906f));
  CollisionTriangle dirt = MakeTri(
      Vec3F(8.33132267f, 4.1875186f, 89.8378601f),
      Vec3F(13.4836855f, 4.1875186f, 90.2841949f),
      Vec3F(19.019783f, 4.19974661f, 97.1494217f));
  if (rock.n.y < 0.0f) {
    rock = MakeTri(rock.a, rock.c, rock.b);
  }
  if (neighbor.n.y < 0.0f) {
    neighbor = MakeTri(neighbor.a, neighbor.c, neighbor.b);
  }
  if (dirt.n.y < 0.0f) {
    dirt = MakeTri(dirt.a, dirt.c, dirt.b);
  }
  CollisionTriangle soup[3] = {rock, neighbor, dirt};

  Vec3F heading(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F sph_off = heading * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F peak_sph = peak + sph_off;
  MovingSphere at_peak(peak_sph, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SphereTriangleContact(at_peak, rock).hit,
      "At the hover peak the offset sphere is not sitting, so lift<=0 "
      "used to DropRideSphere");
  float sd = rock.SignedDistance(peak_sph);
  TEST_CHECK_(sd > radius && (sd - radius) < kMaxHover * units,
      "The sphere is still on the outside of the rock within hover, "
      "sd=%f radius=%f", sd, radius);

  float gnd_peak = HeightBelowFloors(peak.x, peak.z,
      peak.y + 1.2f * units, radius, soup, 3);
  float hover = gnd_peak + kHoverHeight * units;
  TEST_CHECK_(fabsf(hover - peak.y) < 0.03f,
      "The logged peak must be the hover-spring target, hover=%f peak=%f "
      "gnd=%f", hover, peak.y, gnd_peak);

  float gnd_move = HeightBelowFloors(dropped.x, dropped.z,
      peak.y + 1.2f * units, radius, soup, 3);
  float min_y = gnd_move + 1.35f * units;
  float ceiling = gnd_move + kMaxHover * units;
  bool same = (peak.y - gnd_move) < kMaxHover * units * 3.0f;
  bool old_in_band = same && peak.y >= min_y && peak.y <= ceiling;
  TEST_CHECK_(!old_in_band,
      "The tight ceiling band is what treated this reverse as a fall, "
      "gnd=%f min=%f peak=%f ceil=%f",
      gnd_move, min_y, peak.y, ceiling);
  TEST_CHECK_(same,
      "The craft is still on the same surface and must not be dropped, "
      "gnd=%f peak=%f", gnd_move, peak.y);

  // Dropping the offset sphere onto the rock lands near the logged slam.
  float drop_len = peak_sph.y - (peak_sph.y - 2.0f);
  MovingSphere down(peak_sph, radius, Vec3F(0.0f, -drop_len, 0.0f));
  SphereTriangleHit drop_hit = SweptSphereVsTriangle(down, rock);
  TEST_CHECK_(drop_hit.hit && drop_hit.time < 1.0f,
      "DropRideSphere from the peak must hit the rock");
  float dt = drop_hit.time * 0.9f;
  if (dt < 0.0f) {
    dt = 0.0f;
  }
  Vec3F sat = peak_sph + Vec3F(0.0f, -drop_len * dt, 0.0f);
  Vec3F after_drop = sat - sph_off;
  TEST_CHECK_(fabsf(after_drop.y - dropped.y) < 0.04f,
      "The logged slam is DropRideSphere onto the offset sphere's rock, "
      "got y=%f logged=%f", after_drop.y, dropped.y);
}

void test_hover_racer_log_tiny_forward_hover_dip() {
  // New-session log t=69.64..70.04: standing at (4.780, 6.701, 93.523),
  // a small forward nudge, Y climbs 0.005 then lift goes negative and Y
  // drops to 6.639, then the spring climbs back. A vertical sample at the
  // craft XZ is below the sit pose, so hover+0.35u extra damping treated
  // the craft as too high the moment it left sit.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float kMaxHover = 2.2f;
  const float length = 0.776229f;
  const float yaw = 0.549734771f;
  Vec3F rest(4.77962112f, 6.70127916f, 93.5233002f);
  Vec3F peak(4.78457499f, 6.70613527f, 93.5289764f);
  Vec3F dipped(4.80102396f, 6.63940144f, 93.5558167f);
  TEST_CHECK_(peak.y - rest.y < 0.01f,
      "The climb before the dip is a tiny forward follow, dy=%f",
      peak.y - rest.y);
  TEST_CHECK_(peak.y - dipped.y > 0.05f && peak.y - dipped.y < 0.08f,
      "The logged dip must be about 0.067, dy=%f", peak.y - dipped.y);
  float dxz = sqrtf((dipped.x - rest.x) * (dipped.x - rest.x)
      + (dipped.z - rest.z) * (dipped.z - rest.z));
  TEST_CHECK_(dxz < 0.05f,
      "The logged dip is a small forward move, dxz=%f", dxz);

  CollisionTriangle slope = MakeTri(
      Vec3F(4.40158939f, 4.1875186f, 90.7114258f),
      Vec3F(4.86948061f, 6.40157127f, 93.6529465f),
      Vec3F(10.5962973f, 6.40157127f, 96.0796432f));
  if (slope.n.y < 0.0f) {
    slope = MakeTri(slope.a, slope.c, slope.b);
  }
  CollisionTriangle soup[1] = {slope};

  Vec3F heading(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F sph_off = heading * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F rest_sph = rest + sph_off;
  float sd = slope.SignedDistance(rest_sph);
  TEST_CHECK_(sd > 0.0f && sd < radius * 1.2f,
      "At rest the sphere is still on the slope skin, sd=%f radius=%f",
      sd, radius);

  float gnd = HeightBelowFloors(peak.x, peak.z, peak.y + 1.2f * units,
      radius, soup, 1);
  float hover = gnd + kHoverHeight * units;
  float extra = hover + 0.35f * units;
  TEST_CHECK_(fabsf(hover - dipped.y) < 0.05f,
      "The logged dip lands on the vertical hover sample, hover=%f "
      "dipped=%f gnd=%f", hover, dipped.y, gnd);
  TEST_CHECK_(peak.y > extra,
      "The old extra-damping test fired: peak=%f extra=%f", peak.y, extra);

  // A drop of kMaxHover from the ride sphere still hits the slope, so the
  // craft is not airborne. Extra damping must not yank it to the vertical
  // sample.
  Vec3F peak_sph = peak + sph_off;
  float probe = kMaxHover * units;
  MovingSphere down(peak_sph, radius, Vec3F(0.0f, -probe, 0.0f));
  SphereTriangleHit floor = SweptSphereVsTriangle(down, slope);
  TEST_CHECK_(floor.hit && floor.time < 1.0f,
      "The ride sphere still sees the slope under a kMaxHover drop, "
      "hit=%d t=%f", floor.hit, floor.time);
}

// True when a kMaxHover-style drop from the ride sphere still hits a floor.
static bool SphereDropHitsFloor(const Vec3F &center, float radius, float probe,
    const CollisionTriangle *tris, Si32 count) {
  if (probe <= 1e-4f) {
    return false;
  }
  MovingSphere sph(center, radius, Vec3F(0.0f, -probe, 0.0f));
  for (Si32 i = 0; i < count; ++i) {
    if (tris[i].n.y < 0.45f) {
      continue;
    }
    SphereTriangleHit hit = SweptSphereVsTriangle(sph, tris[i]);
    if (hit.hit && hit.time < 1.0f) {
      return true;
    }
  }
  return false;
}

void test_hover_racer_log_downhill_stair_shake() {
  // New-session log t=34.51..35.97: downhill along yaw=7.185 at ~1.7-1.9.
  // Y stuck at 6.5612 while XZ moved, then dropped in ~0.045 stairs every
  // ~0.13s as lift ramped 0 -> -0.65 and snapped back to 0. Treating a
  // kMaxHover floor probe like sitting zeroed the hover spring, so the
  // craft flew level until the slope fell out of the probe, extra damping
  // yanked down, the probe hit again, repeat.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float kMaxHover = 2.2f;
  const float length = 0.776229f;
  const float yaw = 7.18459272f;
  const float slope = 0.16132f;
  const float dt = 0.016f;
  const float speed = 1.73053694f;
  Vec3F plateau(16.0958557f, 6.56121826f, 52.6059456f);
  Vec3F end_pos(18.1920567f, 6.12999725f, 54.2646255f);
  TEST_CHECK_(plateau.y - end_pos.y > 0.40f && plateau.y - end_pos.y < 0.46f,
      "The logged downhill must drop about 0.43, dy=%f",
      plateau.y - end_pos.y);
  float path_xz = sqrtf((end_pos.x - plateau.x) * (end_pos.x - plateau.x)
      + (end_pos.z - plateau.z) * (end_pos.z - plateau.z));
  TEST_CHECK_(path_xz > 2.5f && path_xz < 2.9f,
      "The logged downhill XZ is about 2.67, xz=%f", path_xz);

  Vec3F heading(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F right(cosf(yaw), 0.0f, -sinf(yaw));
  float gnd0 = plateau.y - kHoverHeight * units;
  auto plane_pt = [&](float along, float side) {
    Vec3F p = Vec3F(plateau.x, 0.0f, plateau.z) + heading * along
        + right * side;
    p.y = gnd0 - slope * along;
    return p;
  };
  CollisionTriangle floor = MakeTri(plane_pt(-3.0f, -8.0f),
      plane_pt(10.0f, -8.0f), plane_pt(3.0f, 8.0f));
  if (floor.n.y < 0.0f) {
    floor = MakeTri(floor.a, floor.c, floor.b);
  }
  TEST_CHECK_(floor.n.y / Length(floor.n) > 0.45f,
      "The downhill fixture must be a driveable floor, n.y=%f",
      floor.n.y / Length(floor.n));
  CollisionTriangle soup[1] = {floor};

  Vec3F sph_off = heading * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F start_sph = plateau + sph_off;
  MovingSphere at_start(start_sph, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SphereTriangleContact(at_start, floor).hit,
      "At the logged plateau the offset sphere is not sitting");
  float probe = kMaxHover * units;
  TEST_CHECK_(SphereDropHitsFloor(start_sph, radius, probe, soup, 1),
      "A kMaxHover drop from the plateau still hits the slope, so the old "
      "SeesFloor branch zeroed lift and skipped the spring");

  float gnd = HeightBelowFloors(plateau.x, plateau.z,
      plateau.y + 1.2f * units, radius, soup, 1);
  float hover0 = gnd + kHoverHeight * units;
  TEST_CHECK_(fabsf(hover0 - plateau.y) < 0.03f,
      "The logged plateau is the hover target on this slope, hover=%f "
      "y=%f gnd=%f", hover0, plateau.y, gnd);

  struct DownhillRun {
    float y_at_064;
    float hover_at_064;
    float y_end;
    float hover_end;
    Si32 lift0_frames;
    Si32 stair_resets;
    float min_lift;
  };
  auto simulate = [&](bool kill_spring_when_sees) {
    Vec3F pos = plateau;
    float lift = 0.0f;
    DownhillRun run;
    run.y_at_064 = plateau.y;
    run.hover_at_064 = hover0;
    run.lift0_frames = 0;
    run.stair_resets = 0;
    run.min_lift = 0.0f;
    bool was_neg = false;
    const Si32 kFrames = 90;
    for (Si32 i = 0; i < kFrames; ++i) {
      float ground = HeightBelowFloors(pos.x, pos.z, pos.y + 1.2f * units,
          radius, soup, 1);
      float hover = ground + kHoverHeight * units;
      bool same = (pos.y - ground) < kMaxHover * units * 3.0f;
      Vec3F sph = pos + sph_off;
      bool sitting = SphereTriangleContact(
          MovingSphere(sph, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f)),
          floor).hit;
      bool sees = SphereDropHitsFloor(sph, radius, probe, soup, 1);
      if (kill_spring_when_sees && (sitting || sees)) {
        if (lift < 0.0f) {
          lift = 0.0f;
        }
      } else if (sitting) {
        if (lift < 0.0f) {
          lift = 0.0f;
        }
      } else {
        float ground_sph = HeightBelowFloors(sph.x, sph.z,
            sph.y + 1.2f * units, radius, soup, 1);
        float hover_sph = ground_sph + kHoverHeight * units;
        float err = hover_sph - pos.y;
        if (!same) {
          err = 0.0f;
        }
        // Crest: the sphere is already on top, so do not yank toward a
        // lower sample behind the craft. Downhill: sphere ground is lower
        // and the spring still follows.
        if (ground_sph + 1.0e-3f >= ground) {
          if (err < 0.0f) {
            err = 0.0f;
          }
          if (lift < 0.0f) {
            lift = 0.0f;
          }
        }
        lift += err * 22.0f * dt;
        lift -= lift * 5.5f * dt;
        if (same && pos.y > hover_sph + 0.35f * units && !sees) {
          lift -= 16.0f * units * dt;
        }
      }
      pos = pos + heading * (speed * dt);
      pos.y += lift * dt;
      float min_y = HeightBelowFloors(pos.x, pos.z, pos.y + 1.2f * units,
          radius, soup, 1) + 1.35f * units;
      if (pos.y < min_y) {
        pos.y = min_y;
        if (lift < 0.0f) {
          lift = 0.0f;
        }
      }
      if (fabsf(lift) < 1e-4f) {
        run.lift0_frames += 1;
      }
      if (lift < run.min_lift) {
        run.min_lift = lift;
      }
      if (was_neg && lift > -1e-4f) {
        run.stair_resets += 1;
      }
      was_neg = lift < -0.05f;
      float t = dt * static_cast<float>(i + 1);
      if (fabsf(t - 0.64f) <= dt * 0.51f) {
        run.y_at_064 = pos.y;
        run.hover_at_064 = hover;
      }
    }
    run.y_end = pos.y;
    run.hover_end = HeightBelowFloors(pos.x, pos.z, pos.y + 1.2f * units,
        radius, soup, 1) + kHoverHeight * units;
    return run;
  };

  DownhillRun old_run = simulate(true);
  TEST_CHECK_(fabsf(old_run.y_at_064 - plateau.y) < 0.005f,
      "SeesFloor killing the spring flies level: y=%f start=%f",
      old_run.y_at_064, plateau.y);
  TEST_CHECK_(hover0 - old_run.hover_at_064 > 0.10f,
      "The slope under that level flight must have dropped, hover=%f "
      "start=%f", old_run.hover_at_064, hover0);
  TEST_CHECK_(old_run.lift0_frames > 40,
      "The old policy spends most of the downhill at lift=0, frames=%d",
      old_run.lift0_frames);
  TEST_CHECK_(old_run.stair_resets >= 1,
      "After the floor falls out of kMaxHover the spring yanks and "
      "SeesFloor snaps lift back to 0, resets=%d", old_run.stair_resets);

  DownhillRun fixed = simulate(false);
  TEST_CHECK_(plateau.y - fixed.y_at_064 > 0.05f,
      "The hover spring must follow the falling target, y=%f start=%f",
      fixed.y_at_064, plateau.y);
  TEST_CHECK_(fixed.y_at_064 - fixed.hover_at_064 < 0.15f,
      "Followed Y must stay near hover, y=%f hover=%f",
      fixed.y_at_064, fixed.hover_at_064);
  TEST_CHECK_(fixed.min_lift < -0.05f,
      "Downhill hover must keep negative lift, min=%f", fixed.min_lift);
  TEST_CHECK_(fixed.stair_resets == 0,
      "Following the slope must not snap lift to 0 each step, resets=%d",
      fixed.stair_resets);
  TEST_CHECK_(plateau.y - fixed.y_end > 0.25f,
      "After 1.4s the craft must have descended with the slope, dy=%f",
      plateau.y - fixed.y_end);
}

void test_hover_racer_log_crest_fall_instead_of_level() {
  // New-session log t=167.34: climbed the hill at (-76.49, 9.072, -21.64)
  // to the ridge, then in 0.37s Y slammed to 8.939 with lift to -0.55.
  // Approach 17042 and plateau 14407 share the high edge at y=8.703.
  // A vertical sample at the craft XZ can still hit the steep face while
  // the offset sphere is already over the top. Following that lower hover
  // yanks the craft back down the hill instead of rolling onto the level.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float kMaxHover = 2.2f;
  const float length = 0.776229f;
  const float yaw = 10.5985184f;
  const float dt = 0.016f;
  Vec3F peak(-76.4902496f, 9.07220936f, -21.6412392f);
  Vec3F dropped(-76.5291214f, 8.93892479f, -21.6575317f);
  TEST_CHECK_(peak.y - dropped.y > 0.12f && peak.y - dropped.y < 0.15f,
      "The logged crest slam must be about 0.133, dy=%f",
      peak.y - dropped.y);

  CollisionTriangle slope = MakeTri(
      Vec3F(-72.7282562f, 8.70340347f, -27.6387997f),
      Vec3F(-76.8840714f, 8.70340347f, -21.2898083f),
      Vec3F(-73.0484543f, 6.46007729f, -23.2444916f));
  CollisionTriangle plateau = MakeTri(
      Vec3F(-76.8840714f, 8.70340347f, -21.2898083f),
      Vec3F(-72.7282562f, 8.70340347f, -27.6387997f),
      Vec3F(-76.4116516f, 7.36986589f, -31.3163109f));
  if (slope.n.y < 0.0f) {
    slope = MakeTri(slope.a, slope.c, slope.b);
  }
  if (plateau.n.y < 0.0f) {
    plateau = MakeTri(plateau.a, plateau.c, plateau.b);
  }
  TEST_CHECK_(slope.n.y / Length(slope.n) > 0.45f
          && slope.n.y / Length(slope.n) < 0.85f,
      "The logged approach must be a steep floor, n.y=%f",
      slope.n.y / Length(slope.n));
  TEST_CHECK_(plateau.n.y / Length(plateau.n) > 0.9f,
      "The logged top must be a nearly-flat level, n.y=%f",
      plateau.n.y / Length(plateau.n));
  CollisionTriangle soup[2] = {slope, plateau};

  Vec3F heading(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F sph_off = heading * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F peak_sph = peak + sph_off;
  MovingSphere at_peak(peak_sph, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SphereTriangleContact(at_peak, slope).hit
          && !SphereTriangleContact(at_peak, plateau).hit,
      "At the logged crest peak the sphere is not sitting on either face");
  float probe = kMaxHover * units;
  TEST_CHECK_(SphereDropHitsFloor(peak_sph, radius, probe, soup, 2),
      "A kMaxHover drop from the crest still hits a floor");

  float gnd_craft = HeightBelowFloors(peak.x, peak.z,
      peak.y + 1.2f * units, radius, soup, 2);
  float gnd_sph = HeightBelowFloors(peak_sph.x, peak_sph.z,
      peak_sph.y + 1.2f * units, radius, soup, 2);
  float hover_craft = gnd_craft + kHoverHeight * units;
  float hover_sph = gnd_sph + kHoverHeight * units;
  TEST_CHECK_(peak.y > hover_craft + 0.05f,
      "The logged peak sits above the craft-XZ hover, y=%f hover=%f gnd=%f",
      peak.y, hover_craft, gnd_craft);
  TEST_CHECK_(gnd_sph + 1.0e-3f >= gnd_craft,
      "This is a crest: the sphere's floor is not downhill of the craft, "
      "gnd_sph=%f gnd_craft=%f", gnd_sph, gnd_craft);

  // Old downhill-follow: always pull toward hover. At the crest that is
  // a yank down onto the approach / off the level.
  float lift = 0.0f;
  Vec3F pos = peak;
  for (Si32 i = 0; i < 24; ++i) {
    float ground = HeightBelowFloors(pos.x, pos.z, pos.y + 1.2f * units,
        radius, soup, 2);
    float hover = ground + kHoverHeight * units;
    bool same = (pos.y - ground) < kMaxHover * units * 3.0f;
    float err = same ? hover - pos.y : 0.0f;
    lift += err * 22.0f * dt;
    lift -= lift * 5.5f * dt;
    pos = pos + heading * (0.15f * dt);
    pos.y += lift * dt;
  }
  TEST_CHECK_(peak.y - pos.y > 0.08f,
      "Following craft-XZ hover at the crest must drop Y, dy=%f y=%f",
      peak.y - pos.y, pos.y);
  TEST_CHECK_(pos.y < hover_sph + 0.04f,
      "The old yank lands near or below the top hover, y=%f hover_sph=%f",
      pos.y, hover_sph);

  // Fixed: only follow a falling hover when the sphere's floor is lower
  // (true downhill). At a crest keep Y on the top.
  lift = 0.0f;
  pos = peak;
  for (Si32 i = 0; i < 24; ++i) {
    Vec3F sph = pos + sph_off;
    float ground_craft = HeightBelowFloors(pos.x, pos.z,
        pos.y + 1.2f * units, radius, soup, 2);
    float ground = HeightBelowFloors(sph.x, sph.z, sph.y + 1.2f * units,
        radius, soup, 2);
    float hover = ground + kHoverHeight * units;
    bool same = (pos.y - ground) < kMaxHover * units * 3.0f;
    float err = same ? hover - pos.y : 0.0f;
    if (ground + 1.0e-3f >= ground_craft) {
      if (err < 0.0f) {
        err = 0.0f;
      }
      if (lift < 0.0f) {
        lift = 0.0f;
      }
    }
    lift += err * 22.0f * dt;
    lift -= lift * 5.5f * dt;
    pos = pos + heading * (0.15f * dt);
    pos.y += lift * dt;
  }
  TEST_CHECK_(peak.y - pos.y < 0.05f,
      "A crest must not yank Y down the approach, dy=%f y=%f",
      peak.y - pos.y, pos.y);
  TEST_CHECK_(pos.y > dropped.y + 0.05f,
      "The craft must stay above the logged slam onto the approach, "
      "y=%f slammed=%f", pos.y, dropped.y);
}

// The craft's leftover-velocity clip. A floor, a ceiling or a driveable slope
// keeps the move in its plane; a wall only pushes it out horizontally, so the
// remainder cannot be turned into a hop up a steep face. tangent picks the
// fixed wall rule: cancel the whole into-wall component instead of only the
// XZ part of the move.
static Vec3F ClipSlideRule(const Vec3F &remain, const Vec3F &n,
    bool tangent) {
  float vn = Dot(n, remain);
  if (vn >= 0.0f) {
    return remain;
  }
  if (n.y >= 0.45f || n.y <= -0.45f) {
    return remain - n * vn;
  }
  Vec3F n_xz(n.x, 0.0f, n.z);
  float nl = Length(n_xz);
  if (nl < 1e-5f) {
    return remain;
  }
  n_xz = n_xz / nl;
  Vec3F out = remain;
  if (tangent) {
    float push = vn / nl;
    out.x -= n_xz.x * push;
    out.z -= n_xz.z * push;
    return out;
  }
  float vxz = n_xz.x * remain.x + n_xz.z * remain.z;
  if (vxz < 0.0f) {
    out.x -= n_xz.x * vxz;
    out.z -= n_xz.z * vxz;
  }
  return out;
}

static Vec3F ClipAllSlides(Vec3F vel, const Vec3F *ns, Si32 count,
    bool tangent) {
  for (Si32 it = 0; it < 8; ++it) {
    bool any = false;
    for (Si32 i = 0; i < count; ++i) {
      Vec3F before = vel;
      vel = ClipSlideRule(vel, ns[i], tangent);
      if (LengthSquared(vel - before) > 1e-20f) {
        any = true;
      }
    }
    if (!any) {
      break;
    }
  }
  return vel;
}

// The craft's sweep-and-slide: clip the wish against every plane it already
// sits on, stop at 0.9 of the first hit, clip the remainder against that
// plane too, and repeat up to five times.
static Vec3F SweepAndSlide(const Vec3F &start, const Vec3F &wish,
    float radius, const CollisionTriangle *soup, Si32 count,
    const Vec3F *sits, Si32 sit_count, bool tangent,
    Vec3F *out_wall = nullptr) {
  Vec3F ns[8];
  Si32 nns = 0;
  for (Si32 i = 0; i < sit_count && nns < 8; ++i) {
    ns[nns] = sits[i];
    ++nns;
  }
  Vec3F at = start;
  Vec3F remain = ClipAllSlides(wish, ns, nns, tangent);
  if (out_wall != nullptr) {
    *out_wall = Vec3F(0.0f, 0.0f, 0.0f);
  }
  for (Si32 depth = 0; depth < 5; ++depth) {
    if (LengthSquared(remain) <= 1e-12f) {
      break;
    }
    MovingSphere sph(at, radius, remain);
    SphereTriangleHit cur = SweptSphereVsTriangles(sph, soup, count);
    // The craft learns about a wall from this first hit alone.
    if (depth == 0 && out_wall != nullptr && cur.hit && cur.time < 1.0f) {
      Vec3F n = cur.normal;
      float nl = Length(n);
      if (nl > 1e-8f) {
        n = n / nl;
        Vec3F n_xz(n.x, 0.0f, n.z);
        if (n.y < 0.45f && Length(n_xz) > 1e-5f) {
          *out_wall = Normalize(n_xz);
        }
      }
    }
    if (!cur.hit || cur.time >= 1.0f) {
      at = at + remain;
      break;
    }
    at = at + remain * (cur.time * 0.9f);
    float time_left = 1.0f - cur.time;
    if (time_left <= 0.15f) {
      break;
    }
    remain = remain * time_left;
    if (nns < 8) {
      float nl = Length(cur.normal);
      ns[nns] = (nl > 1e-8f) ? cur.normal / nl : Vec3F(0.0f, 1.0f, 0.0f);
      ++nns;
    }
    remain = ClipAllSlides(remain, ns, nns, tangent);
  }
  return at;
}

// The planes the ride sphere already rests on, gathered the way the craft
// gathers them: floors it sits on, the underside of rock it touches, and,
// once walls are held as contacts, the faces it rests against sideways.
static Si32 RestingPlanes(const Vec3F &center, float radius,
    const CollisionTriangle *soup, Si32 count, bool hold_walls, Vec3F *out) {
  const float skin = 0.06f * 0.140449f;
  Si32 n = 0;
  for (Si32 i = 0; i < count && n < 8; ++i) {
    const CollisionTriangle &tri = soup[i];
    Vec3F face = tri.n / Length(tri.n);
    MovingSphere sph(center, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
    bool touch = SphereTriangleContact(sph, tri).hit;
    if (face.y >= 0.45f) {
      bool sit = touch;
      if (!sit && fabsf(tri.SignedDistance(center)) <= radius * 1.05f) {
        Vec3F below = center;
        below.y -= radius * 3.0f;
        sit = LineSegmentPiercesTriangle(center, below, tri, nullptr, nullptr);
      }
      if (sit) {
        out[n++] = face;
      }
      continue;
    }
    if (face.y <= -0.45f) {
      if (touch) {
        out[n++] = Normalize(center - tri.ClosestPoint(center));
      }
      continue;
    }
    if (!hold_walls
        || Length(center - tri.ClosestPoint(center)) > radius + skin) {
      continue;
    }
    Vec3F n_xz(face.x, 0.0f, face.z);
    if (Length(n_xz) < 1e-5f) {
      continue;
    }
    n_xz = Normalize(n_xz);
    if (tri.SignedDistance(center) < 0.0f) {
      n_xz = n_xz * -1.0f;
    }
    out[n++] = n_xz;
  }
  return n;
}

void test_hover_racer_log_wall_corner_dead_stop() {
  // End of the log: from t=140.53 to t=146.55 the craft stood at
  // (81.8646, 5.8239, -49.4292) with full throttle and speed charging back
  // up to 1.96, and the position never changed by a single bit. The ride
  // sphere is against rock face 17683 and rides slope 15708. The slope is
  // not a contact, but a downward segment crosses it inside the sit slab,
  // so its face normal clips the wish -- and that clip tilts the wish
  // upward. The wall's normal dips below the horizon, so the XZ-only wall
  // clip left that rise pointing into the wall. The sphere touches the
  // wall, so every sweep froze at t=0 and all five slides moved nothing.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float length = 0.776229f;
  const float yaw = -1.29992986f;
  const float speed = 1.96254206f;
  const float lift = 0.00145309512f;
  const float dt = 0.012f;
  Vec3F pos(81.8645935f, 5.82387304f, -49.429184f);

  CollisionTriangle slope = MakeTri(
      Vec3F(79.2673874f, 6.0873094f, -49.9565239f),
      Vec3F(87.2729034f, 4.7537723f, -45.3195953f),
      Vec3F(83.7074585f, 4.7537723f, -52.6726303f));
  CollisionTriangle wall_low = MakeTri(
      Vec3F(82.3298798f, 5.8710408f, -50.3318481f),
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(81.5146866f, 5.4753814f, -49.6385269f));
  CollisionTriangle wall_flat = MakeTri(
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(80.6708298f, 5.7100129f, -49.3927574f),
      Vec3F(81.5146866f, 5.4753814f, -49.6385269f));
  CollisionTriangle soup[3] = {slope, wall_low, wall_flat};
  Vec3F slope_n = slope.n / Length(slope.n);
  Vec3F wall_n = wall_flat.n / Length(wall_flat.n);
  Vec3F low_n = wall_low.n / Length(wall_low.n);
  TEST_CHECK_(slope_n.y > 0.9f,
      "The logged slope must be a driveable floor, n.y=%f", slope_n.y);
  TEST_CHECK_(wall_n.y < 0.0f && wall_n.y > -0.45f,
      "The logged rock face must be a wall whose normal dips below the "
      "horizon, n.y=%f", wall_n.y);
  TEST_CHECK_(low_n.y > -0.45f && low_n.y < 0.45f,
      "The neighboring face must be a wall too, n.y=%f", low_n.y);

  Vec3F heading(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F sph_off = heading * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F ride = pos + sph_off;

  // The slope is a sit constraint through the pierce branch, not a contact.
  MovingSphere resting(ride, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(!SphereTriangleContact(resting, slope).hit,
      "The ride sphere does not touch the slope it rides");
  float slope_sd = slope.SignedDistance(ride);
  TEST_CHECK_(slope_sd > 0.0f && slope_sd <= radius * 1.05f,
      "The slope must be inside the sit slab, sd=%f slab=%f", slope_sd,
      radius * 1.05f);
  Vec3F below = ride;
  below.y -= radius * 3.0f;
  TEST_CHECK_(LineSegmentPiercesTriangle(ride, below, slope, nullptr,
      nullptr),
      "A downward segment must cross the slope, which is what makes it a "
      "sit constraint");
  // The wall is a contact, so any move into it freezes the sweep at t=0.
  TEST_CHECK_(SphereTriangleContact(
      MovingSphere(ride, radius, Vec3F(0.0f, 0.0f, 0.0f)), wall_flat).hit,
      "The ride sphere must be touching the wall");

  Vec3F wish = heading * (speed * dt) + Vec3F(0.0f, lift * dt, 0.0f);
  float wish_xz = sqrtf(wish.x * wish.x + wish.z * wish.z);
  TEST_CHECK_(fabsf(wish_xz - 0.0235505f) < 1e-4f,
      "The logged wish is about 0.02355 of XZ, got %f", wish_xz);
  TEST_CHECK_(fabsf(wish.y) < 1e-4f,
      "The logged wish is level, y=%f", wish.y);

  Vec3F on_slope = ClipSlideRule(wish, slope_n, false);
  TEST_CHECK_(on_slope.y > 0.004f,
      "Clipping the wish into the slope must tilt it upward, y=%f",
      on_slope.y);
  float sqz = wall_n.x * on_slope.x + wall_n.z * on_slope.z;
  float sy = wall_n.y * on_slope.y;
  TEST_CHECK_(sy < -1e-5f,
      "The rise must point into the wall, n.y*wish.y=%f", sy);

  Vec3F old_clip = ClipSlideRule(on_slope, wall_n, false);
  float old_xz = wall_n.x * old_clip.x + wall_n.z * old_clip.z;
  TEST_CHECK_(fabsf(old_xz) < 1e-4f,
      "The XZ-only clip must cancel the XZ part of the move, xz dot=%f",
      old_xz);
  float old_dot = Dot(wall_n, old_clip);
  TEST_CHECK_(old_dot < -1e-5f,
      "The XZ-only clip must still leave the remainder pointing into the "
      "wall, n.remain=%f (xz part %f, y part %f)", old_dot, sqz, sy);
  TEST_CHECK_(fabsf(old_dot) > 1e-3f * Length(old_clip),
      "That leftover is far above a parallel-slide rounding leak: "
      "|n.remain|=%f |remain|=%f", fabsf(old_dot), Length(old_clip));
  MovingSphere old_along(ride, radius, old_clip);
  SphereTriangleHit frozen = SweptSphereVsTriangle(old_along, wall_flat);
  TEST_CHECK_(frozen.hit && frozen.time < 1e-4f,
      "So the swept test freezes the XZ-only remainder at t=0, got hit=%d "
      "t=%f", frozen.hit, frozen.time);

  Vec3F new_clip = ClipSlideRule(on_slope, wall_n, true);
  TEST_CHECK_(fabsf(Dot(wall_n, new_clip)) < 1e-6f,
      "The fixed wall clip must leave the remainder in the wall plane, "
      "n.remain=%f", Dot(wall_n, new_clip));
  TEST_CHECK_(fabsf(new_clip.y - on_slope.y) < 1e-7f,
      "The fixed wall clip must not touch the vertical motion, y=%f was %f",
      new_clip.y, on_slope.y);
  MovingSphere new_along(ride, radius, new_clip);
  SphereTriangleHit slides = SweptSphereVsTriangle(new_along, wall_flat);
  TEST_CHECK_(!slides.hit || slides.time > 1e-4f,
      "The in-plane remainder must slide along the wall, got hit=%d t=%f",
      slides.hit, slides.time);

  // A horizontal remainder must be clipped exactly as before.
  Vec3F level(-0.02f, 0.0f, 0.006f);
  Vec3F level_old = ClipSlideRule(level, wall_n, false);
  Vec3F level_new = ClipSlideRule(level, wall_n, true);
  TEST_CHECK_(Length(level_new - level_old) < 1e-6f,
      "For a level move the fixed rule must match the old one, old=(%f,%f,%f)"
      " new=(%f,%f,%f)", level_old.x, level_old.y, level_old.z, level_new.x,
      level_new.y, level_new.z);
  // A wall may still not lift the remainder up a steep face.
  Vec3F steep_n = Normalize(Vec3F(0.9f, 0.4f, 0.0f));
  Vec3F falling(-0.02f, -0.03f, 0.0f);
  Vec3F kept = ClipSlideRule(falling, steep_n, true);
  TEST_CHECK_(fabsf(kept.y - falling.y) < 1e-7f,
      "A steep face must not turn a fall into a climb, y=%f was %f", kept.y,
      falling.y);

  Vec3F sits[1] = {slope_n};
  Vec3F stuck = SweepAndSlide(ride, wish, radius, soup, 3, sits, 1, false);
  Vec3F stuck_moved = stuck - ride;
  float stuck_xz = sqrtf(stuck_moved.x * stuck_moved.x
      + stuck_moved.z * stuck_moved.z);
  TEST_CHECK_(stuck_xz < 1e-5f,
      "The logged dead stop: five slides against the wall move nothing, "
      "xz=%f wish_xz=%f", stuck_xz, wish_xz);

  Vec3F at = ride;
  float least = wish_xz;
  const Si32 kFrames = 8;
  for (Si32 i = 0; i < kFrames; ++i) {
    Vec3F next = SweepAndSlide(at, wish, radius, soup, 3, sits, 1, true);
    Vec3F step = next - at;
    float step_xz = sqrtf(step.x * step.x + step.z * step.z);
    if (step_xz < least) {
      least = step_xz;
    }
    at = next;
    MovingSphere inside(at, radius * 0.99f, Vec3F(0.0f, 0.0f, 0.0f));
    TEST_CHECK_(!SphereTriangleContact(inside, wall_flat).hit
            && !SphereTriangleContact(inside, wall_low).hit,
        "Sliding along the wall must not enter it, frame %d at (%f,%f,%f)",
        i, at.x, at.y, at.z);
  }
  TEST_CHECK_(least > 0.85f * wish_xz,
      "Every frame along the wall must keep most of the wish, least=%f "
      "wish_xz=%f", least, wish_xz);
  Vec3F went = at - ride;
  float went_xz = sqrtf(went.x * went.x + went.z * went.z);
  TEST_CHECK_(went_xz > 0.85f * wish_xz * static_cast<float>(kFrames),
      "The craft must travel along the wall instead of standing still, "
      "xz=%f over %d frames", went_xz, kFrames);

  // Holding the wall the sphere rests on as a standing contact, which is what
  // stops the shake under a ledge, must not bring this slide back to a stop.
  Vec3F held = ride;
  float held_least = wish_xz;
  for (Si32 i = 0; i < kFrames; ++i) {
    Vec3F planes[8];
    Si32 pn = RestingPlanes(held, radius, soup, 3, true, planes);
    TEST_CHECK_(pn >= 1,
        "The slope under the sphere must always be a plane it rides, "
        "count=%d frame %d", pn, i);
    Vec3F next = SweepAndSlide(held, wish, radius, soup, 3, planes, pn, true);
    Vec3F step = next - held;
    float step_xz = sqrtf(step.x * step.x + step.z * step.z);
    if (step_xz < held_least) {
      held_least = step_xz;
    }
    held = next;
    MovingSphere inside(held, radius * 0.99f, Vec3F(0.0f, 0.0f, 0.0f));
    TEST_CHECK_(!SphereTriangleContact(inside, wall_flat).hit
            && !SphereTriangleContact(inside, wall_low).hit,
        "A held wall must not push the sphere into it, frame %d at "
        "(%f,%f,%f)", i, held.x, held.y, held.z);
  }
  TEST_CHECK_(held_least > 0.5f * wish_xz,
      "A held wall may only shave the into-wall part off the slide, "
      "least=%f wish_xz=%f", held_least, wish_xz);
}

void test_hover_racer_log_wall_ledge_shake() {
  // End of the log: from t=106.4 to t=111.7 the craft stood at
  // (81.5288, 5.9006, -49.1947) at full throttle, never moved more than a
  // thousandth of a unit, and its speed pulsed between 0.05 and 0.34 about
  // ten times a second. The ride sphere rides slope 15708, rests sideways
  // against rock face 17683 and touches the underside of ledge 17682 above
  // it. A wall is only known from the sweep, which reports nothing while the
  // sphere already rests on the face, so the nose was aimed into the rock on
  // most steps and along it on the rest. Under the ledge the wish is
  // projected onto the crease of slope and ledge, and there those two aims
  // slide the craft in opposite directions: it crept into the rock for six
  // steps and jumped back on the seventh, over and over.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float length = 0.776229f;
  const float yaw = -2.55004454f;
  const float dt = 0.0124f;
  const float speed = 0.33f;
  Vec3F pos(81.5289154f, 5.90056276f, -49.1949234f);

  CollisionTriangle slope = MakeTri(
      Vec3F(79.2673874f, 6.0873094f, -49.9565239f),
      Vec3F(87.2729034f, 4.7537723f, -45.3195953f),
      Vec3F(83.7074585f, 4.7537723f, -52.6726303f));
  CollisionTriangle ledge = MakeTri(
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(81.0615387f, 6.8326497f, -48.7671852f),
      Vec3F(80.6708298f, 5.7100129f, -49.3927574f));
  CollisionTriangle wall = MakeTri(
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(80.6708298f, 5.7100129f, -49.3927574f),
      Vec3F(81.5146866f, 5.4753814f, -49.6385269f));
  CollisionTriangle soup[3] = {slope, ledge, wall};
  Vec3F slope_n = slope.n / Length(slope.n);
  Vec3F ledge_n = ledge.n / Length(ledge.n);
  Vec3F wall_n = wall.n / Length(wall.n);
  TEST_CHECK_(slope_n.y > 0.9f, "The logged slope must be a floor, n.y=%f",
      slope_n.y);
  TEST_CHECK_(ledge_n.y < -0.45f,
      "The rock above must count as a ceiling, whose whole plane holds the "
      "move, n.y=%f", ledge_n.y);
  TEST_CHECK_(wall_n.y > -0.45f && wall_n.y < 0.45f,
      "The rock ahead must count as a wall, n.y=%f", wall_n.y);

  Vec3F nose(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F ride = pos + nose * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F wall_xz = Normalize(Vec3F(wall_n.x, 0.0f, wall_n.z));
  float rest_d = Length(ride - wall.ClosestPoint(ride));
  TEST_CHECK_(fabsf(rest_d - radius) < 1e-3f,
      "The logged sphere rests exactly against the wall, d=%f radius=%f",
      rest_d, radius);
  // The ledge is a hair beyond touching at the logged pose and becomes a
  // contact as soon as the craft creeps, which is when it starts holding the
  // move in its plane.
  float ledge_d = Length(ride - ledge.ClosestPoint(ride));
  TEST_CHECK_(ledge_d < radius * 1.02f,
      "The logged sphere is squeezed against the ledge above it, d=%f "
      "radius=%f", ledge_d, radius);
  float into_wall = -(nose.x * wall_xz.x + nose.z * wall_xz.z);
  TEST_CHECK_(into_wall > 0.5f,
      "The logged nose points into the wall, into=%f", into_wall);

  // A sweep that reports nothing is what loses the wall: once the craft aims
  // along the face it rests on, nothing it does enters that face again, so
  // the next step is aimed back into the rock.
  Vec3F along = nose;
  along.x += wall_xz.x * into_wall;
  along.z += wall_xz.z * into_wall;
  along = Normalize(along);
  MovingSphere sliding(ride, radius, along * (speed * dt));
  SphereTriangleHit missed = SweptSphereVsTriangle(sliding, wall);
  TEST_CHECK_(!missed.hit || missed.time >= 1.0f,
      "The sweep must miss the wall the sphere rests on, hit=%d t=%f",
      missed.hit, missed.time);

  const Si32 kFrames = 16;
  float flip_path = 0.0f;
  float flip_net = 0.0f;
  Si32 flips = 0;
  float last_step = 0.0f;
  float span = 0.0f;
  for (Si32 policy = 0; policy < 2; ++policy) {
    bool hold_walls = policy == 1;
    Vec3F at = ride;
    Vec3F contact(0.0f, 0.0f, 0.0f);
    float path = 0.0f;
    float prev = 0.0f;
    float d_min = 1e9f;
    float d_max = -1e9f;
    Si32 turns = 0;
    float step_xz = 0.0f;
    for (Si32 i = 0; i < kFrames; ++i) {
      Vec3F head = nose;
      if (!hold_walls) {
        float into = head.x * contact.x + head.z * contact.z;
        if (into < 0.0f) {
          head.x -= contact.x * into;
          head.z -= contact.z * into;
          float hl = sqrtf(head.x * head.x + head.z * head.z);
          if (hl > 1e-5f) {
            head.x /= hl;
            head.z /= hl;
          }
        }
      }
      Vec3F sits[8];
      Si32 ns = RestingPlanes(at, radius, soup, 3, hold_walls, sits);
      Vec3F wall_hit(0.0f, 0.0f, 0.0f);
      Vec3F next = SweepAndSlide(at, head * (speed * dt), radius, soup, 3,
          sits, ns, true, &wall_hit);
      Vec3F step = next - at;
      step_xz = sqrtf(step.x * step.x + step.z * step.z);
      float along = -(step.x * wall_xz.x + step.z * wall_xz.z);
      path += step_xz;
      if (i > 0 && along * prev < 0.0f) {
        ++turns;
      }
      prev = along;
      at = next;
      float d = Length(at - wall.ClosestPoint(at));
      if (d < d_min) {
        d_min = d;
      }
      if (d > d_max) {
        d_max = d;
      }
      MovingSphere inside(at, radius * 0.99f, Vec3F(0.0f, 0.0f, 0.0f));
      TEST_CHECK_(!SphereTriangleContact(inside, wall).hit
              && !SphereTriangleContact(inside, ledge).hit,
          "Policy %d must not push the sphere into the rock, frame %d at "
          "(%f,%f,%f)", policy, i, at.x, at.y, at.z);
      if (!hold_walls) {
        contact = wall_hit;
      }
    }
    Vec3F net = at - ride;
    if (!hold_walls) {
      flip_path = path;
      flip_net = sqrtf(net.x * net.x + net.z * net.z);
      flips = turns;
      last_step = step_xz;
      span = d_max - d_min;
    } else {
      TEST_CHECK_(turns == 0,
          "Holding the wall must answer the same wish the same way every "
          "frame, direction changed %d times", turns);
      TEST_CHECK_(step_xz < 1e-6f,
          "A craft wedged nose-first must come to rest, last step=%f",
          step_xz);
      TEST_CHECK_(d_max - d_min < 1e-5f,
          "and stay put against the wall, distance to it spans %f",
          d_max - d_min);
    }
  }
  TEST_CHECK_(flips >= 3,
      "The logged shake: aiming at the wall the sweep reports must reverse "
      "the slide over and over, reversals=%d over %d frames", flips, kFrames);
  TEST_CHECK_(flip_path > 3.0f * flip_net,
      "and it must get nowhere while doing it, path=%f net=%f", flip_path,
      flip_net);
  TEST_CHECK_(last_step > 1e-3f,
      "and it must never settle, last step=%f", last_step);
  TEST_CHECK_(span > 1e-4f,
      "and the gap to the wall must saw back and forth, span=%f", span);
}

void test_hover_racer_log_wall_edge_side_snag() {
  // A later session log: from t=154.51 to t=155.46 the craft's XZ position
  // froze bit for bit at (79.4883, -53.6790) for close to a second while
  // full throttle kept climbing the logged speed from 1.68 to 1.94. The
  // ride sphere rests exactly radius away from the crease shared by two
  // rock faces (mesh triangles 17743 and 17718); the wish, after the
  // resting-wall clip, already points away from that crease. The swept
  // vertex/edge test still reported a fresh hit at t~=0 every frame,
  // because HitSegment/HitPoint fed LowestRoot a quadratic whose constant
  // term (offset^2 - radius^2) sits at float noise around zero once the
  // sphere already touches the feature, and LowestRoot returned whichever
  // root landed near t=0 without checking whether the sphere was actually
  // approaching or only just leaving that same contact.
  const float radius = 0.276000023f;
  Vec3F ride(79.5755157f, 5.85876465f, -53.7880974f);
  CollisionTriangle tri_a = MakeTri(
      Vec3F(79.7684097f, 5.7763114f, -53.5974998f),
      Vec3F(79.2322083f, 6.84192085f, -52.2343292f),
      Vec3F(80.2331848f, 7.11174726f, -53.9863014f));
  CollisionTriangle tri_b = MakeTri(
      Vec3F(80.562027f, 5.39735413f, -53.6072464f),
      Vec3F(79.7684097f, 5.7763114f, -53.5974998f),
      Vec3F(80.2331848f, 7.11174726f, -53.9863014f));
  CollisionTriangle soup[2] = {tri_a, tri_b};
  Vec3F na = tri_a.n / Length(tri_a.n);
  Vec3F nb = tri_b.n / Length(tri_b.n);
  TEST_CHECK_(na.y > -0.45f && na.y < 0.45f,
      "Face a must be a wall, n.y=%f", na.y);
  TEST_CHECK_(nb.y > -0.45f && nb.y < 0.45f,
      "Face b must be a wall, n.y=%f", nb.y);

  MovingSphere resting(ride, radius * 1.01f, Vec3F(0.0f, 0.0f, 0.0f));
  SphereTriangleHit touch_a = SphereTriangleContact(resting, tri_a);
  SphereTriangleHit touch_b = SphereTriangleContact(resting, tri_b);
  TEST_CHECK_(touch_a.hit && touch_b.hit,
      "The logged pose must already rest on both faces of the crease");
  TEST_CHECK_(Length(touch_a.point - touch_b.point) < 1e-3f,
      "Both faces' closest points must be the same spot on their shared "
      "edge, a=(%f,%f,%f) b=(%f,%f,%f)", touch_a.point.x, touch_a.point.y,
      touch_a.point.z, touch_b.point.x, touch_b.point.y, touch_b.point.z);
  float d = Length(ride - touch_a.point);
  TEST_CHECK_(fabsf(d - radius) < 1e-3f,
      "The sphere must sit exactly radius away from the shared edge, d=%f "
      "radius=%f", d, radius);

  // This is what the resting-wall clip in ResolveCraftSphere leaves after
  // clipping the throttle wish against the four nearby wall constraints:
  // already pointing away from the crease, not into it.
  Vec3F remain(0.0121073937f, -0.00267693913f, -0.0246515777f);
  TEST_CHECK_(Dot(na, remain) >= 0.0f,
      "The logged remainder must already point away from face a, n.v=%f",
      Dot(na, remain));
  TEST_CHECK_(Dot(nb, remain) >= 0.0f,
      "The logged remainder must already point away from face b, n.v=%f",
      Dot(nb, remain));

  MovingSphere leaving(ride, radius, remain);
  SphereTriangleHit hit_a = SweptSphereVsTriangle(leaving, tri_a);
  SphereTriangleHit hit_b = SweptSphereVsTriangle(leaving, tri_b);
  TEST_CHECK_(!hit_a.hit || hit_a.time > 1e-3f,
      "A velocity already leaving face a must not freeze the sweep at "
      "t=0, got hit=%d t=%f", hit_a.hit, hit_a.time);
  TEST_CHECK_(!hit_b.hit || hit_b.time > 1e-3f,
      "A velocity already leaving face b must not freeze the sweep at "
      "t=0, got hit=%d t=%f", hit_b.hit, hit_b.time);

  // The same pose, run through the craft's own sweep-and-slide (a sweep
  // plus up to four slides, exactly what ResolveCraftSphere does within a
  // single frame) repeatedly with the same wish, must not stand still: it
  // is what read as the craft snagging sideways on the rock and stalling
  // in place while the throttle kept building speed for nothing.
  float wish_xz = sqrtf(remain.x * remain.x + remain.z * remain.z);
  Vec3F at = ride;
  float least_xz = wish_xz;
  const Si32 kFrames = 8;
  for (Si32 i = 0; i < kFrames; ++i) {
    Vec3F next = SweepAndSlide(at, remain, radius, soup, 2, nullptr, 0,
        true);
    Vec3F step = next - at;
    float step_xz = sqrtf(step.x * step.x + step.z * step.z);
    if (step_xz < least_xz) {
      least_xz = step_xz;
    }
    at = next;
  }
  TEST_CHECK_(least_xz > 0.5f * wish_xz,
      "Every frame sliding along the crease must keep most of the wish, "
      "least_xz=%f wish_xz=%f", least_xz, wish_xz);
  Vec3F went = at - ride;
  float went_xz = sqrtf(went.x * went.x + went.z * went.z);
  TEST_CHECK_(went_xz > 0.5f * wish_xz * static_cast<float>(kFrames),
      "The craft must travel along the crease instead of standing still, "
      "xz=%f over %d frames", went_xz, kFrames);
}

void test_swept_sphere_hits_ceiling_from_below() {
  // Ceiling at y=2 with n pointing down. Sitting under it and moving up
  // must hit at t=0. Clipping only the XZ of that normal is a no-op, so
  // the remainder used to keep +Y and the center pierced the roof.
  CollisionTriangle ceil = MakeTri(Vec3F(-4.0f, 2.0f, -4.0f),
      Vec3F(4.0f, 2.0f, -4.0f), Vec3F(0.0f, 2.0f, 4.0f));
  TEST_CHECK_(ceil.n.y < -0.45f,
      "The ceiling fixture must face down, n.y=%f", ceil.n.y);
  const float radius = 1.0f;
  Vec3F start(0.0f, 1.0f, 0.0f);
  MovingSphere sitting(start, radius, Vec3F(0.0f, 0.0f, 0.0f));
  TEST_CHECK_(SphereTriangleContact(sitting, ceil).hit,
      "The sphere must be sitting under the ceiling");

  Vec3F wish(0.2f, 0.5f, 0.1f);
  MovingSphere into(start, radius, wish);
  SphereTriangleHit hit = SweptSphereVsTriangle(into, ceil);
  TEST_CHECK_(hit.hit && hit.time < 1e-3f,
      "Moving up into a ceiling the sphere is sitting under must freeze "
      "at t=0, got hit=%d t=%f", hit.hit, hit.time);

  Vec3F n = ceil.n / Length(ceil.n);
  float vn = Dot(n, wish);
  TEST_CHECK_(vn < 0.0f, "The upward wish must point into the ceiling, vn=%f",
      vn);
  Vec3F n_xz(n.x, 0.0f, n.z);
  TEST_CHECK_(Length(n_xz) < 1e-5f,
      "A flat ceiling has no XZ normal, so a wall clip would leave +Y");
  Vec3F clipped = wish - n * vn;
  TEST_CHECK_(fabsf(clipped.y) < 1e-5f,
      "3D clip against the ceiling must kill upward motion, y=%f", clipped.y);
  TEST_CHECK_(fabsf(Dot(n, clipped)) < 1e-5f,
      "Clipped remainder must lie in the ceiling plane, n·remain=%f",
      Dot(n, clipped));

  MovingSphere along(start, radius, clipped);
  SphereTriangleHit slide = SweptSphereVsTriangle(along, ceil);
  TEST_CHECK_(!slide.hit || slide.time > 1e-4f,
      "The in-plane remainder must slide under the ceiling, not freeze, "
      "got hit=%d t=%f", slide.hit, slide.time);

  float pierce_t = 1.0f;
  TEST_CHECK_(!LineSegmentPiercesTriangle(start, start + clipped, ceil,
      &pierce_t, nullptr),
      "The 3D-clipped remainder must not carry the center through the "
      "ceiling, pierce t=%f", pierce_t);

  Vec3F below(0.0f, 0.0f, 0.0f);
  MovingSphere rise(below, radius, Vec3F(0.0f, 3.0f, 0.0f));
  SphereTriangleHit skin = SweptSphereVsTriangle(rise, ceil);
  TEST_CHECK_(skin.hit && skin.time > 0.2f && skin.time < 0.4f,
      "A rise from below must hit when the skin meets the ceiling, got "
      "hit=%d t=%f", skin.hit, skin.time);
}

void test_classify_sphere_pass_through() {
  CollisionTriangle floor = MakeTri(Vec3F(-4.0f, 0.0f, -4.0f),
      Vec3F(4.0f, 0.0f, -4.0f), Vec3F(0.0f, 0.0f, 4.0f));
  if (floor.n.y < 0.0f) {
    floor = MakeTri(floor.a, floor.c, floor.b);
  }
  TEST_CHECK_(floor.n.y > 0.45f, "The floor fixture must face up, n.y=%f",
      floor.n.y);

  MovingSphere along(Vec3F(0.0f, 1.0f, 0.0f), 1.0f, Vec3F(0.5f, 0.0f, 0.0f));
  SpherePassThrough slide = ClassifySpherePassThrough(along, floor);
  TEST_CHECK_(!slide.passed,
      "Sitting on a floor and sliding along it must not be a pass-through, "
      "center_pierce=%d entered=%d", slide.center_pierce,
      slide.entered_without_hit);

  MovingSphere land(Vec3F(0.0f, 2.0f, 0.0f), 1.0f, Vec3F(0.0f, -1.0f, 0.0f));
  SpherePassThrough landing = ClassifySpherePassThrough(land, floor);
  TEST_CHECK_(!landing.passed,
      "Coming to rest on the skin must not count as going through, "
      "center_pierce=%d entered=%d sweep=%d t=%f", landing.center_pierce,
      landing.entered_without_hit, landing.sweep.hit, landing.sweep.time);

  MovingSphere drop(Vec3F(0.0f, 2.0f, 0.0f), 1.0f, Vec3F(0.0f, -4.0f, 0.0f));
  SpherePassThrough through = ClassifySpherePassThrough(drop, floor);
  TEST_CHECK_(through.passed && through.center_pierce,
      "A drop that carries the center through the floor must be a "
      "pass-through, passed=%d pierce=%d", through.passed,
      through.center_pierce);

  const float radius = 0.276000023f;
  Vec3F start(21.3660736f, 7.0842123f, 68.9416199f);
  Vec3F vel(0.0f, -2.03896618f, 0.0f);
  CollisionTriangle slope = MakeTri(
      Vec3F(19.312149f, 4.1875186f, 72.4813919f),
      Vec3F(22.6897907f, 6.95716333f, 71.8824158f),
      Vec3F(21.4869881f, 7.48888588f, 66.4704208f));
  MovingSphere logged(start, radius, vel);
  SpherePassThrough log_pass = ClassifySpherePassThrough(logged, slope);
  TEST_CHECK_(log_pass.passed && log_pass.center_pierce,
      "The logged rock-slope drop must classify as a pass-through");
  TEST_CHECK_(log_pass.sweep.hit && log_pass.sweep.time < 1.0f,
      "The sweep must still stop that drop, hit=%d t=%f",
      log_pass.sweep.hit, log_pass.sweep.time);

  CollisionTriangle ceil = MakeTri(Vec3F(-4.0f, 2.0f, -4.0f),
      Vec3F(4.0f, 2.0f, -4.0f), Vec3F(0.0f, 2.0f, 4.0f));
  TEST_CHECK_(ceil.n.y < -0.45f, "The ceiling fixture must face down, n.y=%f",
      ceil.n.y);
  MovingSphere rise(Vec3F(0.0f, 0.0f, 0.0f), 1.0f, Vec3F(0.0f, 5.0f, 0.0f));
  SpherePassThrough roof = ClassifySpherePassThrough(rise, ceil);
  TEST_CHECK_(roof.passed && roof.center_pierce,
      "A rise that carries the center through a ceiling must be a "
      "pass-through, passed=%d pierce=%d", roof.passed, roof.center_pierce);

  // SPHERE PIERCE entered_without_hit on a parallel slide: sd did not
  // change, the sphere only acquired skin contact. That is not going
  // through the polygon.
  CollisionTriangle graze = MakeTri(
      Vec3F(8.33132267f, 4.1875186f, 89.8378601f),
      Vec3F(10.5962973f, 6.40157127f, 96.0796432f),
      Vec3F(14.9325733f, 5.83451462f, 91.7753906f));
  Vec3F gfrom(13.0511856f, 6.01273394f, 92.4283371f);
  Vec3F gvel(-0.00632667542f, 0.000114440918f, 0.00386810303f);
  MovingSphere grazing(gfrom, 0.276000023f, gvel);
  SpherePassThrough graze_pass = ClassifySpherePassThrough(grazing, graze);
  TEST_CHECK_(!graze_pass.passed,
      "A parallel slide that only acquires skin contact must not be a "
      "pass-through, entered=%d d0=%f", graze_pass.entered_without_hit,
      graze.SignedDistance(gfrom));
}

// The logged corner shake (a player bounced off a rock at (80.9, -54.0),
// then the camera and craft visibly juddered for about a second) turned out
// to need the real hover_racer collision code to reproduce: a hand-rolled
// model of the sweep-and-slide loop here only ever hit the corner's wall
// faces, never its neighboring ceiling overhangs, and could not reproduce
// the yaw flips this test was built to catch. hover_racer/main.cpp now has
// its own HOVER_SELFTEST=corner_shake regression check that drives the real
// ResolveCraftSphere through the logged pose instead.

// engine/physics_* below is the new kinematic physics module: a broad-phase
// grid over a static triangle soup, a per-body contact manifold persisted
// by feature id, a block solver for the resulting speculative constraints,
// and a sphere body controller and world facade built on top. The tests
// that follow check the module's own pieces first, then re-run the three
// real Hover Racer log fixtures above -- corner dead stop, edge side snag,
// ledge shake -- through PhysicsWorld instead of the hand-rolled
// sweep-and-slide loop, to confirm the new engine does not reintroduce the
// bugs those fixtures were built to catch.

void test_physics_collide_soup_broadphase_completeness() {
  // A mesh of 128 small triangles plus two outlier triangles well outside
  // the main patch, indexed by a deliberately coarse grid (8 cells across
  // a span of about 21 units, so each cell covers roughly 2.6 units and
  // several triangles land in the same bin). Any broad-phase that only
  // ever looks at a single cell, mishandles a query box that straddles a
  // cell boundary, or visits a triangle more than once would be caught by
  // comparing against a brute-force scan of every triangle's own AABB.
  std::vector<Vec3F> pa, pb, pc;
  std::vector<PhysicsMaterial> mats;
  const Si32 kGrid = 8;
  for (Si32 gz = 0; gz < kGrid; ++gz) {
    for (Si32 gx = 0; gx < kGrid; ++gx) {
      float x0 = static_cast<float>(gx);
      float z0 = static_cast<float>(gz);
      float y00 = 0.3f * sinf(static_cast<float>(gx) * 0.7f
          + static_cast<float>(gz) * 0.5f);
      float y10 = 0.3f * sinf(static_cast<float>(gx + 1) * 0.7f
          + static_cast<float>(gz) * 0.5f);
      float y01 = 0.3f * sinf(static_cast<float>(gx) * 0.7f
          + static_cast<float>(gz + 1) * 0.5f);
      float y11 = 0.3f * sinf(static_cast<float>(gx + 1) * 0.7f
          + static_cast<float>(gz + 1) * 0.5f);
      Vec3F p00(x0, y00, z0);
      Vec3F p10(x0 + 1.0f, y10, z0);
      Vec3F p01(x0, y01, z0 + 1.0f);
      Vec3F p11(x0 + 1.0f, y11, z0 + 1.0f);
      pa.push_back(p00);
      pb.push_back(p10);
      pc.push_back(p11);
      pa.push_back(p00);
      pb.push_back(p11);
      pc.push_back(p01);
    }
  }
  Si32 main_count = static_cast<Si32>(pa.size());
  pa.push_back(Vec3F(20.0f, 0.0f, 20.0f));
  pb.push_back(Vec3F(21.0f, 0.0f, 20.0f));
  pc.push_back(Vec3F(20.0f, 0.0f, 21.0f));
  pa.push_back(Vec3F(20.0f, 0.0f, 22.0f));
  pb.push_back(Vec3F(21.0f, 0.0f, 22.0f));
  pc.push_back(Vec3F(20.0f, 0.0f, 23.0f));
  mats.assign(pa.size(), PhysicsMaterial());

  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 8);
  TEST_CHECK_(soup.TriangleCount() == static_cast<Si32>(pa.size()),
      "Every well-formed triangle must survive Build, got %d want %d",
      soup.TriangleCount(), static_cast<Si32>(pa.size()));

  auto BruteForce = [&](const Bound3F &box) {
    std::vector<Si32> expected;
    for (Si32 i = 0; i < soup.TriangleCount(); ++i) {
      const CollisionTriangle &t = soup.Triangle(i);
      Bound3F tb(t.a, t.a);
      tb = Include(tb, t.b);
      tb = Include(tb, t.c);
      if (Overlap(tb, box)) {
        expected.push_back(i);
      }
    }
    return expected;
  };

  Bound3F boxes[5] = {
      Bound3F(1.5f, 3.5f, -10.0f, 10.0f, 1.5f, 3.5f),
      Bound3F(-1.0f, 9.0f, -10.0f, 10.0f, -1.0f, 9.0f),
      Bound3F(19.5f, 21.5f, -1.0f, 1.0f, 19.5f, 21.5f),
      Bound3F(12.0f, 14.0f, -1.0f, 1.0f, 12.0f, 14.0f),
      Bound3F(3.9f, 4.1f, -10.0f, 10.0f, 3.9f, 4.1f),
  };
  std::vector<Si32> box0_expected;
  for (Si32 b = 0; b < 5; ++b) {
    std::vector<Si32> expected = BruteForce(boxes[b]);
    if (b == 0) {
      box0_expected = expected;
    }
    std::vector<bool> seen(static_cast<size_t>(soup.TriangleCount()), false);
    Si32 visits = 0;
    soup.ForEachNear(boxes[b], [&](Si32 idx) {
      TEST_CHECK_(!seen[static_cast<size_t>(idx)],
          "box %d: triangle %d must be visited at most once", b, idx);
      seen[static_cast<size_t>(idx)] = true;
      ++visits;
    });
    for (Si32 idx : expected) {
      TEST_CHECK_(seen[static_cast<size_t>(idx)],
          "box %d: broad-phase missed triangle %d, whose AABB truly "
          "overlaps the query box", b, idx);
    }
    TEST_CHECK_(visits >= static_cast<Si32>(expected.size()),
        "box %d: got %d candidates, brute force found %d must-haves", b,
        visits, static_cast<Si32>(expected.size()));
  }
  TEST_CHECK_(!box0_expected.empty(),
      "box 0 must overlap at least one triangle of the main patch");
  TEST_CHECK_(main_count > 0, "the main patch must be non-empty");

  // Negative control: box 0 straddles more than one grid cell (it spans a
  // known bin boundary near x=z=2.6), so the triangles it truly overlaps
  // are not all reachable from the single cell holding just its corner.
  // A broad-phase that only ever consulted one bin would fail this box.
  Bound3F corner_probe(boxes[0].min_x, boxes[0].min_x, boxes[0].min_y,
      boxes[0].min_y, boxes[0].min_z, boxes[0].min_z);
  std::vector<Si32> from_one_cell;
  soup.ForEachNear(corner_probe, [&](Si32 idx) {
    from_one_cell.push_back(idx);
  });
  TEST_CHECK_(from_one_cell.size() < box0_expected.size(),
      "box 0 must need more than the single cell holding its corner, so "
      "this fixture would fail a broad-phase that only consulted one "
      "bin: one-cell=%d full-box=%d",
      static_cast<int>(from_one_cell.size()),
      static_cast<int>(box0_expected.size()));
}

void test_physics_collide_soup_wide_aabb_not_dropped() {
  // CollideSoup::Build sizes its grid from the mesh's own span, so
  // width_/height_ normally track target_cells_per_axis exactly and every
  // triangle's cell range [CellX(minx), CellX(maxx)] already sits inside
  // [0, width_-1]. But target_cells_per_axis is clamped to at most 4096
  // cells per axis (kMaxCellsPerAxis) while cell_ itself is not adjusted
  // to match, so requesting far more cells than that leaves cell_ tiny
  // relative to width_*cell_: CellX of anything past roughly cell 4096
  // overshoots width_-1. Build() and ForEachNear() used to clamp only the
  // end of each axis that was expected to need it (max(lo, 0) and
  // min(hi, count-1)), leaving the other end unclamped -- so a triangle or
  // query box overshooting on the max side got x1 clamped down while x0
  // stayed above it, and the insertion/visit loop (which only runs while
  // lo <= hi) silently did nothing. A triangle far from the grid's origin,
  // and a large triangle whose own AABB reaches into that same overshoot
  // region, both used to vanish from every query -- not just queries at
  // the far edge, but the far triangle even from a box sitting right on
  // top of it.
  std::vector<Vec3F> pa, pb, pc;
  std::vector<PhysicsMaterial> mats;
  pa.push_back(Vec3F(0.0f, 0.0f, 0.0f));
  pb.push_back(Vec3F(1.0f, 0.0f, 0.0f));
  pc.push_back(Vec3F(0.0f, 0.0f, 1.0f));

  pa.push_back(Vec3F(99.0f, 0.0f, 99.0f));
  pb.push_back(Vec3F(100.0f, 0.0f, 99.0f));
  pc.push_back(Vec3F(99.0f, 0.0f, 100.0f));

  Si32 wide_index = static_cast<Si32>(pa.size());
  pa.push_back(Vec3F(0.5f, 0.0f, 0.5f));
  pb.push_back(Vec3F(99.5f, 0.0f, 0.6f));
  pc.push_back(Vec3F(0.6f, 0.0f, 99.5f));

  mats.assign(pa.size(), PhysicsMaterial());
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 1000000);
  TEST_CHECK_(soup.TriangleCount() == static_cast<Si32>(pa.size()),
      "every well-formed triangle must survive Build, got %d want %d",
      soup.TriangleCount(), static_cast<Si32>(pa.size()));

  auto Found = [&](float x, float z) {
    Bound3F box(x - 0.4f, x + 0.4f, -1.0f, 1.0f, z - 0.4f, z + 0.4f);
    std::vector<Si32> got;
    soup.ForEachNear(box, [&](Si32 idx) { got.push_back(idx); });
    return got;
  };

  std::vector<Si32> far_corner = Found(99.3f, 99.3f);
  TEST_CHECK_(std::find(far_corner.begin(), far_corner.end(), 1)
      != far_corner.end(),
      "a small triangle far from the grid origin must still be found by "
      "a query box sitting right on top of it, found %d candidates",
      static_cast<int>(far_corner.size()));

  std::vector<Si32> wide_far = Found(90.0f, 0.55f);
  TEST_CHECK_(std::find(wide_far.begin(), wide_far.end(), wide_index)
      != wide_far.end(),
      "a large triangle whose own AABB reaches past the grid's clamp "
      "region must still be found near its far end");

  std::vector<Si32> wide_mid = Found(40.0f, 0.55f);
  TEST_CHECK_(std::find(wide_mid.begin(), wide_mid.end(), wide_index)
      != wide_mid.end(),
      "the same large triangle must also be found in the middle of its "
      "own span, not just at one end");
}

void test_physics_manifold_feature_stability() {
  // The manifold's whole job is bookkeeping across steps, independent of
  // how a ContactFeature was computed, so this drives PhysicsManifold
  // directly with hand-built features rather than real triangles.
  PhysicsManifold manifold;
  auto MakeContact = [](Si32 tri_index, ContactFeatureKind kind, Si32 sub,
      float separation) {
    ContactPoint c;
    c.feature.tri_index = tri_index;
    c.feature.kind = kind;
    c.feature.sub = sub;
    c.separation = separation;
    return c;
  };

  // The same feature across several steps must keep aging instead of
  // being recreated, and must carry forward whatever warm_speed a solver
  // stamps onto it after each Update().
  for (Si32 i = 0; i < 4; ++i) {
    std::vector<ContactPoint> fresh;
    fresh.push_back(MakeContact(7, ContactFeatureKind::kFace, 0,
        0.01f * static_cast<float>(i)));
    manifold.Update(&fresh);
    TEST_CHECK_(manifold.Contacts().size() == 1,
        "step %d must keep exactly one contact, got %d", i,
        static_cast<int>(manifold.Contacts().size()));
    TEST_CHECK_(manifold.Contacts()[0].age == i,
        "the same feature id across steps must keep aging instead of "
        "resetting, step %d got age=%d", i, manifold.Contacts()[0].age);
    if (i == 0) {
      TEST_CHECK_(manifold.Contacts()[0].warm_speed == 0.0f,
          "the first time a feature appears it has no warm_speed yet, "
          "got %f", manifold.Contacts()[0].warm_speed);
    } else {
      float want = 10.0f + static_cast<float>(i - 1);
      TEST_CHECK_(fabsf(manifold.Contacts()[0].warm_speed - want) < 1e-6f,
          "step %d must carry over the warm_speed stamped after step %d, "
          "want %f got %f", i, i - 1, want, manifold.Contacts()[0].warm_speed);
    }
    manifold.MutableContacts()[0].warm_speed = 10.0f + static_cast<float>(i);
  }

  // A different triangle -- even one right next door -- is a different
  // feature id and must not inherit age or warm_speed from the old one.
  std::vector<ContactPoint> switched;
  switched.push_back(MakeContact(8, ContactFeatureKind::kFace, 0, 0.0f));
  manifold.Update(&switched);
  TEST_CHECK_(manifold.Contacts()[0].age == 0,
      "a genuinely different feature must not inherit age, got %d",
      manifold.Contacts()[0].age);
  TEST_CHECK_(manifold.Contacts()[0].warm_speed == 0.0f,
      "a genuinely different feature must not inherit warm_speed, got %f",
      manifold.Contacts()[0].warm_speed);

  // A different edge sub-index on the same triangle is also a different
  // feature: sub must matter, not just (tri_index, kind).
  std::vector<ContactPoint> edge_a;
  edge_a.push_back(MakeContact(8, ContactFeatureKind::kEdge, 0, 0.0f));
  manifold.Update(&edge_a);
  manifold.MutableContacts()[0].warm_speed = 42.0f;
  std::vector<ContactPoint> edge_b;
  edge_b.push_back(MakeContact(8, ContactFeatureKind::kEdge, 1, 0.0f));
  manifold.Update(&edge_b);
  TEST_CHECK_(manifold.Contacts()[0].age == 0
          && manifold.Contacts()[0].warm_speed == 0.0f,
      "a different edge sub-index on the same triangle must count as a "
      "different feature, age=%d warm_speed=%f",
      manifold.Contacts()[0].age, manifold.Contacts()[0].warm_speed);

  // Returning to a feature id absent from the immediately preceding step
  // must start over: Update() only ever compares against the previous
  // call's result, it is not a long-lived cache.
  std::vector<ContactPoint> back;
  back.push_back(MakeContact(7, ContactFeatureKind::kFace, 0, 0.0f));
  manifold.Update(&back);
  TEST_CHECK_(manifold.Contacts()[0].age == 0,
      "a feature missing from the immediately preceding step must "
      "restart at age 0 even if it was seen earlier, got %d",
      manifold.Contacts()[0].age);

  // Canonical ordering: several simultaneous contacts must always come
  // out sorted by (tri_index, kind, sub), regardless of the order fed in,
  // so the PGS fallback sees a deterministic order independent of
  // broad-phase traversal.
  std::vector<ContactPoint> multi;
  multi.push_back(MakeContact(5, ContactFeatureKind::kVertex, 2, 0.0f));
  multi.push_back(MakeContact(2, ContactFeatureKind::kFace, 0, 0.0f));
  multi.push_back(MakeContact(5, ContactFeatureKind::kEdge, 0, 0.0f));
  manifold.Update(&multi);
  TEST_CHECK_(manifold.Contacts().size() == 3,
      "all three simultaneous contacts must survive, got %d",
      static_cast<int>(manifold.Contacts().size()));
  TEST_CHECK_(manifold.Contacts()[0].feature.tri_index == 2,
      "contacts must be sorted by tri_index first, got %d",
      manifold.Contacts()[0].feature.tri_index);
  TEST_CHECK_(manifold.Contacts()[1].feature.tri_index == 5
          && manifold.Contacts()[1].feature.kind
              == ContactFeatureKind::kEdge,
      "within the same tri_index, kFace/kEdge/kVertex must come out in "
      "that order");
  TEST_CHECK_(manifold.Contacts()[2].feature.tri_index == 5
          && manifold.Contacts()[2].feature.kind
              == ContactFeatureKind::kVertex,
      "the vertex feature must sort after the edge feature on the same "
      "triangle");
}

void test_physics_solver_corner_order_invariance_and_pgs() {
  // Three mutually perpendicular walls meeting at a cube corner: pressing
  // straight into the corner must stop dead (v=0), and which order the
  // three constraints are listed in must not change the answer, since
  // the block solve enumerates constraint subsets rather than clipping
  // them in sequence.
  SpeculativeConstraint base[3];
  base[0].normal = Vec3F(1.0f, 0.0f, 0.0f);
  base[1].normal = Vec3F(0.0f, 1.0f, 0.0f);
  base[2].normal = Vec3F(0.0f, 0.0f, 1.0f);
  for (Si32 i = 0; i < 3; ++i) {
    base[i].bound = 0.0f;
  }
  Vec3F wish(1.0f, 1.0f, 1.0f);
  Si32 perms[6][3] = {
      {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0},
  };
  for (Si32 p = 0; p < 6; ++p) {
    SpeculativeConstraint cs[3];
    for (Si32 i = 0; i < 3; ++i) {
      cs[i] = base[perms[p][i]];
    }
    ContactSolveResult r = SolveContactVelocity(wish, cs, 3);
    TEST_CHECK_(Length(r.velocity) < 1e-5f,
        "perm %d: pressing straight into a cube corner must stop dead, "
        "got (%f,%f,%f)", p, r.velocity.x, r.velocity.y, r.velocity.z);
    TEST_CHECK_(r.outcome == ContactSolveOutcome::kCorner,
        "perm %d: three simultaneously active constraints must classify "
        "as a corner, got %d", p, static_cast<int>(r.outcome));
    TEST_CHECK_(r.active_mask == 7u,
        "perm %d: all three constraints must be active, mask=%u", p,
        r.active_mask);
  }

  // Moving away from the corner is unconstrained by any of the three
  // faces and must pass through untouched.
  ContactSolveResult free_result = SolveContactVelocity(
      Vec3F(-1.0f, -1.0f, -1.0f), base, 3);
  TEST_CHECK_(Length(free_result.velocity - Vec3F(-1.0f, -1.0f, -1.0f))
          < 1e-5f,
      "moving away from all three faces must pass through unclipped, "
      "got (%f,%f,%f)", free_result.velocity.x, free_result.velocity.y,
      free_result.velocity.z);
  TEST_CHECK_(free_result.outcome == ContactSolveOutcome::kFree,
      "an unconstrained wish must report kFree, got %d",
      static_cast<int>(free_result.outcome));

  // A fourth, redundant constraint (a duplicate of the first face) pushes
  // the count over the exact block solve's limit of three and into the
  // PGS fallback; the corner's answer must not change, and it must not
  // depend on which position in the array the duplicate sits at.
  for (Si32 p = 0; p < 6; ++p) {
    SpeculativeConstraint cs4[4];
    for (Si32 i = 0; i < 3; ++i) {
      cs4[i] = base[perms[p][i]];
    }
    cs4[3] = base[0];
    ContactSolveResult r4 = SolveContactVelocity(wish, cs4, 4);
    TEST_CHECK_(Length(r4.velocity) < 1e-4f,
        "perm %d: PGS with a redundant fourth constraint must still stop "
        "dead at the corner, got (%f,%f,%f)", p, r4.velocity.x,
        r4.velocity.y, r4.velocity.z);
    TEST_CHECK_(r4.outcome == ContactSolveOutcome::kPgs,
        "perm %d: more than three constraints must fall back to PGS, "
        "got %d", p, static_cast<int>(r4.outcome));
  }
}

void test_physics_solver_coplanar_wedge_stops_cleanly() {
  // Three vertical wall normals spread 120 degrees apart in the XZ plane,
  // summing to exactly zero. Because they are coplanar (all n.y == 0),
  // any nonzero horizontal velocity has a strictly positive dot with at
  // least one of them (the three dot products sum to zero, so if none
  // were positive all three would have to be exactly zero, which forces
  // the velocity itself to be the zero vector, since two of the three
  // normals already span the whole XZ plane). So a horizontal wish has
  // exactly one feasible answer -- v=0 -- and any two of the three walls
  // already pin it down (the third is redundant), which is why this
  // comes back as an edge (a one-dimensional feasible line, here the
  // vertical axis the walls have no opinion about) rather than the
  // kDegenerate fallback, which is reserved for configurations where no
  // subset of the active constraints locates a feasible point at all.
  // What actually matters -- and is what the plan's "clean stop, not a
  // direction picked at random" is about -- is that the answer is the
  // same zero vector no matter which order the three constraints are
  // listed in.
  SpeculativeConstraint cs[3];
  cs[0].normal = Vec3F(1.0f, 0.0f, 0.0f);
  cs[1].normal = Vec3F(-0.5f, 0.0f, 0.8660254f);
  cs[2].normal = Vec3F(-0.5f, 0.0f, -0.8660254f);
  cs[0].bound = 0.0f;
  cs[1].bound = 0.0f;
  cs[2].bound = 0.0f;

  Si32 perms[6][3] = {
      {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0},
  };
  Vec3F wish(0.5f, 0.0f, 0.2f);
  for (Si32 p = 0; p < 6; ++p) {
    SpeculativeConstraint ordered[3];
    for (Si32 i = 0; i < 3; ++i) {
      ordered[i] = cs[perms[p][i]];
    }
    ContactSolveResult r = SolveContactVelocity(wish, ordered, 3);
    TEST_CHECK_(Length(r.velocity) < 1e-4f,
        "perm %d: three walls spread 120 degrees apart must stop a "
        "horizontal wish dead, regardless of listing order, got "
        "(%f,%f,%f)", p, r.velocity.x, r.velocity.y, r.velocity.z);
    TEST_CHECK_(r.outcome == ContactSolveOutcome::kEdge,
        "perm %d: this wedge has exactly one feasible line (vertical), "
        "so it must classify as an edge, not fall through to a random "
        "single-face pick, got %d", p, static_cast<int>(r.outcome));
  }

  // The same three walls have no opinion at all about vertical motion
  // (every normal has n.y == 0), so a wish with a vertical component must
  // keep that component exactly while still losing its horizontal part.
  Vec3F rising(0.5f, 2.0f, 0.2f);
  ContactSolveResult vertical = SolveContactVelocity(rising, cs, 3);
  TEST_CHECK_(fabsf(vertical.velocity.y - rising.y) < 1e-3f,
      "vertical motion must survive a wedge built entirely from vertical "
      "walls, wanted y=%f got y=%f", rising.y, vertical.velocity.y);
  TEST_CHECK_(sqrtf(vertical.velocity.x * vertical.velocity.x
      + vertical.velocity.z * vertical.velocity.z) < 1e-3f,
      "the horizontal part must still be killed, got xz=(%f,%f)",
      vertical.velocity.x, vertical.velocity.z);
}

void test_physics_solver_duplicate_constraint() {
  // Two walls meeting at a vertical corner (mirrored across the X axis,
  // so their shared feasible line is the Y axis): pressing straight into
  // the corner in X must stop the horizontal motion dead. Adding an exact
  // duplicate of one of the two constraints must not change that answer.
  SpeculativeConstraint n1;
  n1.normal = Normalize(Vec3F(1.0f, 0.0f, 1.0f));
  n1.bound = 0.0f;
  SpeculativeConstraint n2;
  n2.normal = Normalize(Vec3F(1.0f, 0.0f, -1.0f));
  n2.bound = 0.0f;

  Vec3F wish(1.0f, 0.0f, 0.0f);
  SpeculativeConstraint pair[2] = {n1, n2};
  ContactSolveResult base_result = SolveContactVelocity(wish, pair, 2);
  TEST_CHECK_(Length(base_result.velocity) < 1e-5f,
      "pressing straight into this vertical corner must stop dead, got "
      "(%f,%f,%f)", base_result.velocity.x, base_result.velocity.y,
      base_result.velocity.z);
  TEST_CHECK_(base_result.outcome == ContactSolveOutcome::kEdge,
      "two simultaneously active constraints must classify as an edge, "
      "got %d", static_cast<int>(base_result.outcome));

  SpeculativeConstraint with_dup[3] = {n1, n2, n1};
  ContactSolveResult dup_result = SolveContactVelocity(wish, with_dup, 3);
  TEST_CHECK_(Length(dup_result.velocity - base_result.velocity) < 1e-5f,
      "an exact duplicate of an already-active constraint must not "
      "change the answer, base=(%f,%f,%f) with_dup=(%f,%f,%f)",
      base_result.velocity.x, base_result.velocity.y, base_result.velocity.z,
      dup_result.velocity.x, dup_result.velocity.y, dup_result.velocity.z);

  // A wish that only grazes the corner (already leaving both faces) must
  // pass through unconstrained, with or without the duplicate.
  Vec3F leaving(-1.0f, 0.3f, 0.0f);
  ContactSolveResult base_leave = SolveContactVelocity(leaving, pair, 2);
  ContactSolveResult dup_leave = SolveContactVelocity(leaving, with_dup, 3);
  TEST_CHECK_(Length(base_leave.velocity - leaving) < 1e-5f,
      "leaving both faces must pass through unclipped, got (%f,%f,%f)",
      base_leave.velocity.x, base_leave.velocity.y, base_leave.velocity.z);
  TEST_CHECK_(Length(dup_leave.velocity - base_leave.velocity) < 1e-5f,
      "the duplicate must not change the unconstrained case either, "
      "base=(%f,%f,%f) with_dup=(%f,%f,%f)", base_leave.velocity.x,
      base_leave.velocity.y, base_leave.velocity.z, dup_leave.velocity.x,
      dup_leave.velocity.y, dup_leave.velocity.z);
}

void test_physics_world_fixture_corner_dead_stop() {
  // Same log fixture as test_hover_racer_log_wall_corner_dead_stop, run
  // through PhysicsWorld instead of the hand-rolled sweep-and-slide loop:
  // the craft must keep making real progress along the wall every frame,
  // where the old code's XZ-only wall clip left the whole log frozen at
  // this exact pose for six seconds straight.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float length = 0.776229f;
  const float yaw = -1.29992986f;
  const float speed = 1.96254206f;
  const float lift = 0.00145309512f;
  const float dt = 0.012f;
  Vec3F pos(81.8645935f, 5.82387304f, -49.429184f);

  CollisionTriangle slope = MakeTri(
      Vec3F(79.2673874f, 6.0873094f, -49.9565239f),
      Vec3F(87.2729034f, 4.7537723f, -45.3195953f),
      Vec3F(83.7074585f, 4.7537723f, -52.6726303f));
  CollisionTriangle wall_low = MakeTri(
      Vec3F(82.3298798f, 5.8710408f, -50.3318481f),
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(81.5146866f, 5.4753814f, -49.6385269f));
  CollisionTriangle wall_flat = MakeTri(
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(80.6708298f, 5.7100129f, -49.3927574f),
      Vec3F(81.5146866f, 5.4753814f, -49.6385269f));

  std::vector<Vec3F> pa = {slope.a, wall_low.a, wall_flat.a};
  std::vector<Vec3F> pb = {slope.b, wall_low.b, wall_flat.b};
  std::vector<Vec3F> pc = {slope.c, wall_low.c, wall_flat.c};
  std::vector<PhysicsMaterial> mats(3);

  PhysicsWorld world;
  PhysicsStepConfig config;
  config.skin = 0.06f * units;
  world.SetConfig(config);
  world.SetStaticMesh(pa, pb, pc, mats, 8);

  Vec3F heading(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F sph_off = heading * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  Vec3F ride = pos + sph_off;
  Vec3F wish = heading * speed + Vec3F(0.0f, lift, 0.0f);
  float wish_xz = sqrtf(wish.x * wish.x + wish.z * wish.z) * dt;
  TEST_CHECK_(fabsf(wish_xz - 0.0235505f) < 1e-4f,
      "The logged wish is about 0.02355 of XZ per frame, got %f", wish_xz);

  PhysicsBodyId craft = world.AddSphere(ride, radius);
  world.SetWishVelocity(craft, wish);

  Vec3F at = ride;
  float least_xz = wish_xz;
  const Si32 kFrames = 8;
  for (Si32 i = 0; i < kFrames; ++i) {
    world.Step(dt);
    Vec3F next = world.Position(craft);
    Vec3F step = next - at;
    float step_xz = sqrtf(step.x * step.x + step.z * step.z);
    if (step_xz < least_xz) {
      least_xz = step_xz;
    }
    at = next;
    MovingSphere inside(at, radius * 0.99f, Vec3F(0.0f, 0.0f, 0.0f));
    TEST_CHECK_(!SphereTriangleContact(inside, wall_flat).hit
            && !SphereTriangleContact(inside, wall_low).hit,
        "The new engine must not push the craft into the wall it slides "
        "along, frame %d at (%f,%f,%f)", i, at.x, at.y, at.z);
  }
  TEST_CHECK_(least_xz > 0.85f * wish_xz,
      "Every frame along the wall must keep most of the wish instead of "
      "dead-stopping like the old sweep-and-slide loop, least=%f "
      "wish_xz=%f", least_xz, wish_xz);
  Vec3F went = at - ride;
  float went_xz = sqrtf(went.x * went.x + went.z * went.z);
  TEST_CHECK_(went_xz > 0.85f * wish_xz * static_cast<float>(kFrames),
      "The craft must travel along the wall over %d frames instead of "
      "standing still, xz=%f", kFrames, went_xz);

  // Negative control: the exact same geometry and per-frame displacement
  // still dead-stops the old sweep-and-slide loop, which is the bug this
  // engine exists to remove by construction rather than by another layer
  // of hysteresis.
  CollisionTriangle soup[3] = {slope, wall_low, wall_flat};
  Vec3F slope_n = slope.n / Length(slope.n);
  Vec3F sits[1] = {slope_n};
  Vec3F stuck = SweepAndSlide(ride, wish * dt, radius, soup, 3, sits, 1,
      false);
  Vec3F stuck_moved = stuck - ride;
  float stuck_xz = sqrtf(stuck_moved.x * stuck_moved.x
      + stuck_moved.z * stuck_moved.z);
  TEST_CHECK_(stuck_xz < 1e-5f,
      "The old code's dead stop at this exact pose is the regression this "
      "test guards against, xz=%f wish_xz=%f", stuck_xz, wish_xz);
}

void test_physics_world_fixture_edge_snag() {
  // Same log fixture as test_hover_racer_log_wall_edge_side_snag: a wish
  // that already points away from both faces of a crease must be carried
  // through essentially unchanged by the new narrow-phase, which tests
  // the closest point on each triangle rather than a swept quadratic
  // whose constant term goes to float noise once the sphere already
  // touches the feature (that primitive-level fix lives in and is tested
  // by sphere_vs_triangle.cpp/test_hover_racer_log_wall_edge_side_snag;
  // this test's job is to confirm the higher-level module built on top of
  // it -- GatherSphereContacts, the manifold, the solver -- does not
  // quietly reintroduce the freeze through some other path).
  const float radius = 0.276000023f;
  Vec3F ride(79.5755157f, 5.85876465f, -53.7880974f);
  CollisionTriangle tri_a = MakeTri(
      Vec3F(79.7684097f, 5.7763114f, -53.5974998f),
      Vec3F(79.2322083f, 6.84192085f, -52.2343292f),
      Vec3F(80.2331848f, 7.11174726f, -53.9863014f));
  CollisionTriangle tri_b = MakeTri(
      Vec3F(80.562027f, 5.39735413f, -53.6072464f),
      Vec3F(79.7684097f, 5.7763114f, -53.5974998f),
      Vec3F(80.2331848f, 7.11174726f, -53.9863014f));

  std::vector<Vec3F> pa = {tri_a.a, tri_b.a};
  std::vector<Vec3F> pb = {tri_a.b, tri_b.b};
  std::vector<Vec3F> pc = {tri_a.c, tri_b.c};
  std::vector<PhysicsMaterial> mats(2);

  PhysicsWorld world;
  PhysicsStepConfig config;
  config.skin = 0.06f * 0.140449f;
  world.SetConfig(config);
  world.SetStaticMesh(pa, pb, pc, mats, 8);
  PhysicsBodyId craft = world.AddSphere(ride, radius);

  Vec3F remain(0.0121073937f, -0.00267693913f, -0.0246515777f);
  float wish_xz = sqrtf(remain.x * remain.x + remain.z * remain.z);
  world.SetWishVelocity(craft, remain);

  Vec3F at = ride;
  float least_xz = wish_xz;
  const Si32 kFrames = 8;
  for (Si32 i = 0; i < kFrames; ++i) {
    world.Step(1.0f);
    Vec3F next = world.Position(craft);
    Vec3F step = next - at;
    float step_xz = sqrtf(step.x * step.x + step.z * step.z);
    if (step_xz < least_xz) {
      least_xz = step_xz;
    }
    at = next;
  }
  TEST_CHECK_(least_xz > 0.9f * wish_xz,
      "A wish already leaving the crease must keep nearly all of itself "
      "every frame instead of snagging like the old swept-edge false "
      "hit, least=%f wish_xz=%f", least_xz, wish_xz);
  Vec3F went = at - ride;
  float went_xz = sqrtf(went.x * went.x + went.z * went.z);
  TEST_CHECK_(went_xz > 0.9f * wish_xz * static_cast<float>(kFrames),
      "The craft must travel almost the full distance over %d frames "
      "instead of snagging on the crease, xz=%f", kFrames, went_xz);

  MovingSphere leaving(ride, radius, remain);
  SphereTriangleHit hit_a = SweptSphereVsTriangle(leaving, tri_a);
  SphereTriangleHit hit_b = SweptSphereVsTriangle(leaving, tri_b);
  TEST_CHECK_(!hit_a.hit || hit_a.time > 1e-3f,
      "leaving face a must not freeze the sweep at t=0, hit=%d t=%f",
      hit_a.hit, hit_a.time);
  TEST_CHECK_(!hit_b.hit || hit_b.time > 1e-3f,
      "leaving face b must not freeze the sweep at t=0, hit=%d t=%f",
      hit_b.hit, hit_b.time);
}

void test_physics_world_fixture_ledge_shake() {
  // Same log fixture as test_hover_racer_log_wall_ledge_shake, run through
  // PhysicsWorld: nose pinned into a rock face while a ledge presses down
  // from above. The old sweep-only wall detection loses the wall the
  // instant the craft's own last move already cleared it, so the very
  // next frame aims back into the rock, back and forth, forever. The new
  // engine keeps that wall (and the ledge) as a standing contact through
  // the manifold instead of only trusting a fresh sweep, so the distance
  // to the wall must move in one direction, never oscillate.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float kHoverHeight = 1.6f;
  const float length = 0.776229f;
  const float yaw = -2.55004454f;
  const float dt = 0.0124f;
  const float speed = 0.33f;
  Vec3F pos(81.5289154f, 5.90056276f, -49.1949234f);

  CollisionTriangle slope = MakeTri(
      Vec3F(79.2673874f, 6.0873094f, -49.9565239f),
      Vec3F(87.2729034f, 4.7537723f, -45.3195953f),
      Vec3F(83.7074585f, 4.7537723f, -52.6726303f));
  CollisionTriangle ledge = MakeTri(
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(81.0615387f, 6.8326497f, -48.7671852f),
      Vec3F(80.6708298f, 5.7100129f, -49.3927574f));
  CollisionTriangle wall = MakeTri(
      Vec3F(81.7932281f, 6.2757416f, -49.6763535f),
      Vec3F(80.6708298f, 5.7100129f, -49.3927574f),
      Vec3F(81.5146866f, 5.4753814f, -49.6385269f));
  CollisionTriangle soup[3] = {slope, ledge, wall};

  std::vector<Vec3F> pa = {slope.a, ledge.a, wall.a};
  std::vector<Vec3F> pb = {slope.b, ledge.b, wall.b};
  std::vector<Vec3F> pc = {slope.c, ledge.c, wall.c};
  std::vector<PhysicsMaterial> mats(3);

  PhysicsWorld world;
  PhysicsStepConfig config;
  config.skin = 0.06f * units;
  world.SetConfig(config);
  world.SetStaticMesh(pa, pb, pc, mats, 8);

  Vec3F nose(sinf(yaw), 0.0f, cosf(yaw));
  Vec3F ride = pos + nose * (length * 0.18f)
      + Vec3F(0.0f, radius - kHoverHeight * units, 0.0f);
  PhysicsBodyId craft = world.AddSphere(ride, radius);
  world.SetWishVelocity(craft, nose * speed);

  const Si32 kFrames = 16;
  float prev_d = Length(ride - wall.ClosestPoint(ride));
  float min_step = 1e9f;
  float max_step = 0.0f;
  Si32 backward = 0;
  for (Si32 i = 0; i < kFrames; ++i) {
    Vec3F before = world.Position(craft);
    world.Step(dt);
    Vec3F after = world.Position(craft);
    Vec3F step = after - before;
    float step_xz = sqrtf(step.x * step.x + step.z * step.z);
    if (step_xz < min_step) {
      min_step = step_xz;
    }
    if (step_xz > max_step) {
      max_step = step_xz;
    }
    float d = Length(after - wall.ClosestPoint(after));
    if (d < prev_d - 1.0e-5f) {
      ++backward;
    }
    prev_d = d;
    MovingSphere inside(after, radius * 0.99f, Vec3F(0.0f, 0.0f, 0.0f));
    TEST_CHECK_(!SphereTriangleContact(inside, wall).hit
            && !SphereTriangleContact(inside, ledge).hit,
        "The new engine must not push the craft into the rock, frame %d "
        "at (%f,%f,%f)", i, after.x, after.y, after.z);
  }
  TEST_CHECK_(backward == 0,
      "Wedged nose-first, the distance to the wall must never step "
      "backward -- that back-and-forth is exactly the logged shake, "
      "backward steps=%d over %d frames", backward, kFrames);
  TEST_CHECK_(max_step < 2.0f * min_step,
      "and the per-frame step size must stay in a tight band instead of "
      "pulsing between near-zero and near-full speed like the logged "
      "shake, min=%f max=%f", min_step, max_step);

  // Negative control: the logged mechanism (drop the wall constraint the
  // instant the last sweep found nothing, so the next frame's nose aims
  // straight back into the rock) still reproduces the shake on this exact
  // pose when run through the old policy-0 loop.
  Vec3F wall_xz = Normalize(Vec3F(wall.n.x, 0.0f, wall.n.z));
  Vec3F at = ride;
  Vec3F contact(0.0f, 0.0f, 0.0f);
  float prev_along = 0.0f;
  Si32 turns = 0;
  for (Si32 i = 0; i < kFrames; ++i) {
    Vec3F head = nose;
    float into = head.x * contact.x + head.z * contact.z;
    if (into < 0.0f) {
      head.x -= contact.x * into;
      head.z -= contact.z * into;
      float hl = sqrtf(head.x * head.x + head.z * head.z);
      if (hl > 1e-5f) {
        head.x /= hl;
        head.z /= hl;
      }
    }
    Vec3F sits[8];
    Si32 ns = RestingPlanes(at, radius, soup, 3, false, sits);
    Vec3F wall_hit(0.0f, 0.0f, 0.0f);
    Vec3F next = SweepAndSlide(at, head * (speed * dt), radius, soup, 3,
        sits, ns, true, &wall_hit);
    Vec3F step = next - at;
    float along = -(step.x * wall_xz.x + step.z * wall_xz.z);
    if (i > 0 && along * prev_along < 0.0f) {
      ++turns;
    }
    prev_along = along;
    at = next;
    contact = wall_hit;
  }
  TEST_CHECK_(turns >= 3,
      "The old sweep-only wall memory must still reverse direction over "
      "and over on this exact pose, reversals=%d over %d frames -- that "
      "is the regression the manifold's standing contact fixes", turns,
      kFrames);
}

void test_physics_sphere_body_reacquires_steep_floor_gap() {
  // Hover Racer's Xcode debug-build session log (build/Debug's log.txt,
  // the last of 56 launches appended to that file) shows the player
  // reversing down a steep slope: has_floor stays true for over a hundred
  // frames while sphere_y descends smoothly, then for exactly one substep
  // (dt=0.0041s, speed=-0.4885 world units/s) has_floor flips to false and
  // never comes back for the rest of the recorded session -- over four
  // more seconds, with the craft's height frozen and then drifting on
  // whatever lift the hover spring supplied, never touching the mesh
  // again. That reads to the player as the craft falling off a step
  // instead of continuing to slide down the hill.
  //
  // GatherSphereContacts already has a fallback for exactly this shape of
  // problem -- a floor triangle whose closest point is a bit further than
  // `reach` away gets a second chance via a straight vertical pierce that
  // probes a full three radii down -- but both that fallback's own gate
  // and the broad-phase query feeding it were still bounded by
  // `radius + reach` (skin plus a single substep's travel: at the logged
  // speed and dt that is on the order of a hundredth of a world unit), so
  // a floor genuinely within the probe's three-radius reach, but past that
  // tight bound, was never offered to the pierce at all. This fixture
  // reproduces that exact substep (same radius, skin, speed and dt as the
  // log) with a flat floor sitting at a gap comfortably inside the
  // pierce's own three-radius reach but well past the old radius+reach
  // gate.
  CollisionTriangle floor_a = MakeTri(
      Vec3F(-5.0f, 0.0f, -5.0f), Vec3F(-5.0f, 0.0f, 5.0f),
      Vec3F(5.0f, 0.0f, 5.0f));
  CollisionTriangle floor_b = MakeTri(
      Vec3F(-5.0f, 0.0f, -5.0f), Vec3F(5.0f, 0.0f, 5.0f),
      Vec3F(5.0f, 0.0f, -5.0f));
  std::vector<Vec3F> pa = {floor_a.a, floor_b.a};
  std::vector<Vec3F> pb = {floor_a.b, floor_b.b};
  std::vector<Vec3F> pc = {floor_a.c, floor_b.c};
  std::vector<PhysicsMaterial> mats(2);
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 8);

  const float radius = 0.276000023f;
  PhysicsStepConfig config;
  config.skin = 0.00842695f;
  const Vec3F wish(-0.5f, 0.0f, 0.0f);
  const float dt = 0.0041f;

  float travel = Length(wish) * dt;
  float reach = config.skin + travel;
  // The gate compares the *plane* distance from the sphere's center
  // (radius + separation for a flat floor straight below), not the bare
  // separation, against radius + reach on the old path and radius * 3 on
  // the fixed one. Pick a separation whose plane distance clears the old
  // gate but not the new one: reach < separation < 2 * radius.
  float min_separation = reach;
  float max_separation = 2.0f * radius;
  TEST_CHECK_(min_separation < max_separation,
      "test setup: reach=%f must leave room under 2*radius=%f",
      min_separation, max_separation);
  float gap = min_separation + 0.5f * (max_separation - min_separation);
  Vec3F center(0.0f, radius + gap, 0.0f);
  float plane_d = center.y;
  TEST_CHECK_(plane_d > radius + reach && plane_d <= radius * 3.0f,
      "test setup: plane_d=%f must sit strictly between the old gate "
      "radius+reach=%f and the pierce's own reach radius*3=%f", plane_d,
      radius + reach, radius * 3.0f);

  std::vector<ContactPoint> contacts;
  SphereBodySupport support;
  GatherSphereContacts(soup, center, radius, wish, dt, config, &contacts,
      &support);
  // What the reacquire has to deliver is the contact: that is what keeps
  // the solver from letting the sphere pass the face, and what the seam
  // assist in StepSphereBody keys off. Support is deliberately not
  // claimed here -- a floor this far out is one the sphere is flying over
  // rather than standing on, and saying otherwise is what froze a craft
  // in mid-air over a slope (see
  // test_physics_sphere_body_hovering_over_floor_is_not_sitting).
  TEST_CHECK_(!contacts.empty(),
      "A floor triangle within the vertical pierce's own three-radius "
      "reach must still produce a contact even though it sits past the "
      "much tighter skin+travel reach -- gap=%f reach=%f 3*radius=%f",
      gap, reach, radius * 3.0f);
  TEST_CHECK_(!support.has_floor,
      "a floor %f world units below the sphere's surface is past the "
      "contact skin %f, so it is not support", gap, config.skin);
  if (!contacts.empty()) {
    TEST_CHECK_(contacts[0].normal.y > 0.99f,
        "the reacquired contact's normal must point straight up off this "
        "flat floor, normal=(%f,%f,%f)", contacts[0].normal.x,
        contacts[0].normal.y, contacts[0].normal.z);
    float expected_separation = gap;
    TEST_CHECK_(
        std::fabs(contacts[0].separation - expected_separation) < 1.0e-3f,
        "the reported separation must match the real gap, got=%f "
        "expected=%f", contacts[0].separation, expected_separation);
  }

  // Negative control: reverting the widened gate (i.e. gating and
  // querying at radius + reach the way the old code did) must reproduce
  // the exact failure this fixture guards against.
  float old_query_reach = radius + reach;
  std::vector<Si32> old_hits;
  soup.ForEachNearSegment(center, center + wish * dt, old_query_reach,
      [&](Si32 idx) { old_hits.push_back(idx); });
  TEST_CHECK_(old_hits.empty(),
      "sanity check: the old radius+reach broad-phase bound must not even "
      "reach this floor, so the fix has to be the widened query, not just "
      "the inner gate -- hits=%d", static_cast<int>(old_hits.size()));
}

void test_physics_sphere_body_hovering_over_floor_is_not_sitting() {
  // Hover Racer's HOVER_SELFTEST=reverse_drop run logs the player's ride
  // sphere frozen at sphere_y=5.3582 from t=2s to the end of the run,
  // while the same sphere dropped down the same column of the same mesh
  // comes to rest 3.4 design units (0.48 world units, nearly two radii)
  // lower: the craft hangs in the air over the slope it is supposed to be
  // rolling down, and nothing ever asks it to descend.
  //
  // The reason is the vertical-pierce fallback in GatherSphereContacts.
  // Probing three radii below the centre is right, and so is the contact
  // it makes down there -- that contact is what stops the solver from
  // letting a fast sphere pass straight through the face. Reporting
  // support from it is not. SphereBodySupport is documented as the face
  // the sphere is "currently touched"-ing, and it exists so a caller can
  // "zero a downward lift once a floor is found"; Hover Racer, like any
  // such caller, stops pushing the craft down the moment it is told there
  // is a floor. A floor almost two radii away then reads as solid ground
  // under a craft that is plainly airborne, and the two decisions
  // deadlock: the game will not descend because it believes it is
  // sitting, and it never starts touching because it never descends.
  CollisionTriangle floor_a = MakeTri(
      Vec3F(-5.0f, 0.0f, -5.0f), Vec3F(-5.0f, 0.0f, 5.0f),
      Vec3F(5.0f, 0.0f, 5.0f));
  CollisionTriangle floor_b = MakeTri(
      Vec3F(-5.0f, 0.0f, -5.0f), Vec3F(5.0f, 0.0f, 5.0f),
      Vec3F(5.0f, 0.0f, -5.0f));
  std::vector<Vec3F> pa = {floor_a.a, floor_b.a};
  std::vector<Vec3F> pb = {floor_a.b, floor_b.b};
  std::vector<Vec3F> pc = {floor_a.c, floor_b.c};
  std::vector<PhysicsMaterial> mats(2);
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 8);

  const float radius = 0.276000023f;
  PhysicsStepConfig config;
  const float dt = 1.0f / 60.0f;
  // The logged gap, in radii: inside the pierce's three-radius probe (so
  // the fallback does fire) and far outside the contact skin (so no
  // reading of "touching" can include it).
  const float gap = 1.7f * radius;
  TEST_CHECK_(gap > config.skin,
      "test setup: gap=%f must be well past the contact skin=%f to count "
      "as plainly airborne", gap, config.skin);
  TEST_CHECK_(radius + gap <= radius * 3.0f,
      "test setup: plane distance %f must stay inside the pierce's own "
      "three-radius probe %f, or the fallback under test never fires",
      radius + gap, radius * 3.0f);

  Vec3F center(0.0f, radius + gap, 0.0f);
  std::vector<ContactPoint> contacts;
  SphereBodySupport support;
  GatherSphereContacts(soup, center, radius, Vec3F(0.0f, 0.0f, 0.0f), dt,
      config, &contacts, &support);
  TEST_CHECK_(!support.has_floor,
      "a floor %f world units (%.2f radii) below the sphere's surface is "
      "not something the sphere rests on, skin=%f", gap, gap / radius,
      config.skin);
  TEST_CHECK_(!contacts.empty(),
      "the far floor must still produce a speculative contact, or nothing "
      "stops a fast sphere from passing through it");
  if (!contacts.empty()) {
    TEST_CHECK_(std::fabs(contacts[0].separation - gap) < 1.0e-3f,
        "the contact must report the real gap, got=%f expected=%f",
        contacts[0].separation, gap);
  }

  // Same sphere actually resting on the same floor: support is exactly
  // what this flag is for, so the strictness above must not have cost it.
  Vec3F resting(0.0f, radius + config.skin * 0.25f, 0.0f);
  contacts.clear();
  support = SphereBodySupport();
  GatherSphereContacts(soup, resting, radius, Vec3F(0.0f, 0.0f, 0.0f), dt,
      config, &contacts, &support);
  TEST_CHECK_(support.has_floor,
      "a sphere sitting within the contact skin of the floor must report "
      "support, gap=%f skin=%f", config.skin * 0.25f, config.skin);

  // The deadlock itself, driven the way Hover Racer drives it: while the
  // engine reports a floor the craft holds its lift at zero, otherwise
  // the lift falls away (UpdateCraft's kAirborneFall, 18 design units per
  // second, which is about 2.5 world units per second squared at Hover
  // Racer's scale). With support claimed from the far pierce, the wish is
  // zero on every step and the sphere never moves at all.
  PhysicsManifold manifold;
  Vec3F pos(0.0f, radius + gap, 0.0f);
  float lift = 0.0f;
  const float fall = 2.5f;
  // Measured off the geometry rather than off the flag under test, so a
  // step that merely claims a floor cannot satisfy this.
  Si32 touched_at = -1;
  const Si32 steps = 120;
  for (Si32 i = 0; i < steps; ++i) {
    SphereStepResult step = StepSphereBody(soup, &manifold, &pos, radius,
        Vec3F(0.0f, lift, 0.0f), dt, config);
    if (touched_at < 0 && pos.y - radius <= config.skin) {
      touched_at = i;
    }
    if (step.support.has_floor) {
      if (lift < 0.0f) {
        lift = 0.0f;
      }
    } else {
      lift -= fall * dt;
    }
  }
  // Free fall over this gap takes sqrt(2 * 0.469 / 2.5) = 0.61s, so a
  // sphere that is allowed to fall has really landed inside 1.5 seconds.
  TEST_CHECK_(touched_at >= 0 && touched_at < 90,
      "the sphere must reach the floor it is hovering over, instead it "
      "was still airborne after %d steps at y=%f (gap %f left)", steps,
      pos.y, pos.y - radius);
  TEST_CHECK_(pos.y <= radius + config.skin,
      "after landing the sphere must rest on the floor, y=%f expected at "
      "most %f", pos.y, radius + config.skin);
  TEST_CHECK_(pos.y > radius - config.max_position_correction,
      "the sphere must not have been pushed through the floor, y=%f",
      pos.y);
}

void test_physics_world_fixture_floor_climb_collapse() {
  // A HOVER_SELFTEST=... style autopilot lap over the real track logged the
  // craft's vertical speed collapsing to exactly zero for one frame every
  // few facets while climbing a low-poly rock slope, then resuming its
  // normal climb rate -- the "trembling while touching the track" the
  // player reported. A logged collapse frame's exact contact set (11
  // facets, separations from 0.007 to 0.08, wish horizontal) fed straight
  // into SolveContactVelocity confirmed 0 is the mathematically correct
  // answer for that discrete set: none of the eleven facets is tight
  // enough to bind the solve, so a purely horizontal wish sails through
  // unredirected. The craft is not blocked, it is floating a hair above
  // the mesh with no penetration for the existing residual-penetration
  // cleanup to act on and nothing else pulling it back down -- so on a
  // stretch of near-flat facets, that hairline gap never closes on its
  // own and the craft coasts at whatever height it happened to reach last,
  // one triangle seam at a time, which is what reads as trembling.
  //
  // Reproduced directly here with a small tessellated flat floor (a
  // faceted mesh's local seams are the point, a single infinite plane
  // would not have any) and the sphere starting at the exact separation
  // logged for the tightest of those eleven facets. Confirmed by hand
  // against this exact fixture that reverting the fix below (physics_sphere_body.cpp's
  // gap-closing pass alongside the existing penetration cleanup) leaves
  // the gap frozen at 0.00744 for as long as the sphere sits over one
  // facet, then frozen again at whatever the next seam happens to leave
  // it at, forever, instead of the fixed code's steady per-step decay
  // asserted below.
  std::vector<Vec3F> pa, pb, pc;
  std::vector<PhysicsMaterial> mats;
  const Si32 kGrid = 6;
  for (Si32 gz = -kGrid; gz < kGrid; ++gz) {
    for (Si32 gx = -kGrid; gx < kGrid; ++gx) {
      Vec3F p00(static_cast<float>(gx), 5.5f, static_cast<float>(gz));
      Vec3F p10(static_cast<float>(gx + 1), 5.5f, static_cast<float>(gz));
      Vec3F p01(static_cast<float>(gx), 5.5f, static_cast<float>(gz + 1));
      Vec3F p11(static_cast<float>(gx + 1), 5.5f, static_cast<float>(gz + 1));
      pa.push_back(p00);
      pb.push_back(p01);
      pc.push_back(p10);
      pa.push_back(p10);
      pb.push_back(p01);
      pc.push_back(p11);
    }
  }
  mats.assign(pa.size(), PhysicsMaterial());
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 8);

  const float radius = 0.276000023f;
  const float dt = 0.0166666675f;
  const float gap = 0.00743940473f;
  Vec3F start(0.3f, 5.5f + radius + gap, 0.3f);

  PhysicsStepConfig config;
  config.skin = 0.06f * 0.140449f;
  config.floor_normal_y = 0.45f;
  Vec3F wish(-2.8664701f, 0.0f, -4.53065062f);

  auto SepAt = [&](const Vec3F &p) {
    return p.y - radius - 5.5f;
  };

  PhysicsManifold manifold;
  Vec3F position = start;
  Si32 frozen_run = 0;
  Si32 worst_frozen_run = 0;
  float prev_sep = gap;
  const Si32 kFrames = 20;
  for (Si32 i = 0; i < kFrames; ++i) {
    StepSphereBody(soup, &manifold, &position, radius, wish, dt, config);
    float sep = SepAt(position);
    if (sep > prev_sep - 1.0e-6f) {
      ++frozen_run;
      if (frozen_run > worst_frozen_run) {
        worst_frozen_run = frozen_run;
      }
    } else {
      frozen_run = 0;
    }
    prev_sep = sep;
  }
  TEST_CHECK_(worst_frozen_run < 4,
      "A hairline gap above a floor facet the craft is not penetrating "
      "must keep shrinking every few frames instead of freezing at a "
      "fixed height for many frames in a row -- that stuck-forever gap is "
      "exactly the logged trembling, longest frozen run=%d frames over %d",
      worst_frozen_run, kFrames);
  float final_sep = SepAt(position);
  TEST_CHECK_(final_sep < 0.6f * gap,
      "The craft must settle much closer to a facet it is already "
      "floating a hair above instead of coasting past it at a fixed "
      "height, start_gap=%f final_sep=%f", gap, final_sep);
}

void test_physics_sphere_body_rolls_over_road_seam_skirt() {
  // Straight out of a Hover Racer log: the craft came to a dead stop in the
  // middle of a flat road and stayed there for six seconds at full throttle,
  // with twelve contacts and a wall reported on level ground. The track mesh
  // splits the road into tiles, and every tile closes its own edge with a
  // skirt that hangs straight down from the surface -- so at a seam two
  // near-vertical faces stand back to back, and their topmost edge sits at
  // exactly the height the ride sphere rolls on. Those faces are neither
  // floor nor ceiling by their own normal, and flattening their contact into
  // the XZ plane (right for a wall beside the sphere, which must not launch
  // the body upward) turned the level seam into a vertical wall as tall as
  // the whole skirt, standing across the road. The real direction out of the
  // contact is nearly straight up, so the fix in GatherSphereContacts keeps
  // it whole whenever the sphere rides over the top of such a face.
  //
  // Geometry, sphere pose and wish below are the logged ones: two road
  // tiles at y=6.09 meeting at z=55.826, each with its own skirt down to
  // y=4.2, one facing +Z and one facing -Z.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float dt = 0.0166666675f;
  const float road_y = 6.09000349f;
  const float seam_z = 55.8258095f;

  // The fixture builder is what keeps the two skirts really being the
  // near-vertical faces the log describes, standing back to back across the
  // seam: a transcription that flipped one of them would otherwise pass no
  // matter what the contact code did.
  const Vec3F up(0.0f, 1.0f, 0.0f);
  const Vec3F toward_z(0.0f, 0.0f, 1.0f);
  const Vec3F away_z(0.0f, 0.0f, -1.0f);
  CollideSoupFixture fixture;
  fixture.AddFacing(
      Vec3F(-76.2699127f, 6.09000969f, 55.3730125f),
      Vec3F(-76.5137024f, 6.09001017f, 55.3653564f),
      Vec3F(-76.507164f, 6.09000349f, 55.8258095f), up);
  fixture.AddFacing(
      Vec3F(-76.2699127f, 6.09000969f, 55.3730125f),
      Vec3F(-76.507164f, 6.09000349f, 55.8258095f),
      Vec3F(-76.2706528f, 6.09000158f, 55.8258934f), up);
  fixture.AddFacing(
      Vec3F(-76.4949417f, 6.09000349f, 55.8269501f),
      Vec3F(-76.4944534f, 6.09000921f, 56.2978592f),
      Vec3F(-76.2528305f, 6.09000921f, 56.289856f), up);
  fixture.AddFacing(
      Vec3F(-76.4949417f, 6.09000349f, 55.8269501f),
      Vec3F(-76.2528305f, 6.09000921f, 56.289856f),
      Vec3F(-76.2614441f, 6.09000015f, 55.8268738f), up);
  fixture.AddFacing(
      Vec3F(-77.3691711f, 4.19974661f, 55.8515434f),
      Vec3F(-76.2706528f, 6.09000158f, 55.8258934f),
      Vec3F(-76.507164f, 6.09000349f, 55.8258095f), toward_z);
  Si32 skirt_far = fixture.AddFacing(
      Vec3F(-77.3672943f, 4.19974613f, 55.810318f),
      Vec3F(-76.4949417f, 6.09000349f, 55.8269501f),
      Vec3F(-76.2614441f, 6.09000015f, 55.8268738f), away_z);
  CollideSoup soup;
  fixture.Build(&soup);
  TEST_CHECK_(fixture.Ok(), "The fixture must describe the logged seam:\n%s",
      fixture.Problems().c_str());

  const CollisionTriangle &far_tri = fixture.Triangle(skirt_far);
  float skirt_top = std::max(std::max(far_tri.a.y, far_tri.b.y), far_tri.c.y);
  TEST_CHECK_(std::fabs(skirt_top - road_y) < 1.0e-3f,
      "The skirt's top edge must sit at the road level the sphere rolls "
      "on, skirt_top=%f road_y=%f", skirt_top, road_y);

  PhysicsStepConfig config;
  config.max_substep_move = 1.15f * units;
  config.skin = 0.06f * units;
  config.floor_normal_y = 0.45f;

  Vec3F start(-76.4620132f, 6.36600637f, 55.8240395f);
  Vec3F wish(-0.286900014f, 0.0f, 3.07990003f);
  float wish_xz = std::sqrt(wish.x * wish.x + wish.z * wish.z) * dt;
  TEST_CHECK_(std::fabs(start.y - radius - road_y) < 1.0e-3f,
      "The sphere must start resting on the road, bottom=%f road_y=%f",
      start.y - radius, road_y);
  TEST_CHECK_(start.z < seam_z && seam_z - start.z < radius,
      "The sphere must start just short of the seam it is about to cross, "
      "z=%f seam_z=%f", start.z, seam_z);

  PhysicsManifold manifold;
  Vec3F at = start;
  float least_xz = wish_xz;
  Si32 wall_frames = 0;
  const Si32 kFrames = 8;
  for (Si32 i = 0; i < kFrames; ++i) {
    Vec3F before = at;
    SphereStepResult r = StepSphereBody(soup, &manifold, &at, radius, wish, dt,
        config);
    if (r.support.has_wall) {
      ++wall_frames;
    }
    Vec3F step = at - before;
    float step_xz = std::sqrt(step.x * step.x + step.z * step.z);
    if (step_xz < least_xz) {
      least_xz = step_xz;
    }
    TEST_CHECK_(std::fabs(at.y - radius - road_y) < 0.1f * units,
        "The sphere must stay on the road across the seam instead of "
        "being launched or sunk, frame %d bottom=%f road_y=%f", i,
        at.y - radius, road_y);
  }
  TEST_CHECK_(wall_frames == 0,
      "A level seam in the middle of a flat road must not be reported as a "
      "wall, %d of %d frames claimed one", wall_frames, kFrames);
  TEST_CHECK_(at.z > seam_z + radius,
      "The sphere must roll clear across the seam instead of stopping dead "
      "in the middle of the road, z=%f seam_z=%f", at.z, seam_z);
  TEST_CHECK_(least_xz > 0.85f * wish_xz,
      "Every frame across the seam must keep most of the wish, least=%f "
      "wish_xz=%f", least_xz, wish_xz);
}

void test_physics_sphere_body_step_above_center_still_blocks() {
  // Guard for the seam fix above: keeping a contact's own direction instead
  // of flattening it into the XZ plane is right only while the sphere is
  // riding over the top of the face. A step whose riser reaches above the
  // sphere's center is a wall, and the sphere must be stopped by it rather
  // than allowed to climb, or the fix would have traded a dead stop in the
  // middle of the road for a craft that walks up every kerb and barrier.
  const float units = 0.140449f;
  const float radius = 0.276000023f;
  const float dt = 0.0166666675f;
  const float floor_y = 6.09f;
  const float step_z = 55.826f;
  const float step_h = 0.5f;

  CollisionTriangle floor_a = MakeTri(
      Vec3F(-77.0f, floor_y, 55.0f),
      Vec3F(-76.0f, floor_y, 55.0f),
      Vec3F(-77.0f, floor_y, step_z));
  CollisionTriangle floor_b = MakeTri(
      Vec3F(-76.0f, floor_y, 55.0f),
      Vec3F(-76.0f, floor_y, step_z),
      Vec3F(-77.0f, floor_y, step_z));
  CollisionTriangle riser_a = MakeTri(
      Vec3F(-77.0f, floor_y, step_z),
      Vec3F(-76.0f, floor_y, step_z),
      Vec3F(-77.0f, floor_y + step_h, step_z));
  CollisionTriangle riser_b = MakeTri(
      Vec3F(-76.0f, floor_y, step_z),
      Vec3F(-76.0f, floor_y + step_h, step_z),
      Vec3F(-77.0f, floor_y + step_h, step_z));
  CollisionTriangle deck_a = MakeTri(
      Vec3F(-77.0f, floor_y + step_h, step_z),
      Vec3F(-76.0f, floor_y + step_h, step_z),
      Vec3F(-77.0f, floor_y + step_h, 56.8f));
  CollisionTriangle deck_b = MakeTri(
      Vec3F(-76.0f, floor_y + step_h, step_z),
      Vec3F(-76.0f, floor_y + step_h, 56.8f),
      Vec3F(-77.0f, floor_y + step_h, 56.8f));

  std::vector<Vec3F> pa = {floor_a.a, floor_b.a, riser_a.a, riser_b.a,
      deck_a.a, deck_b.a};
  std::vector<Vec3F> pb = {floor_a.b, floor_b.b, riser_a.b, riser_b.b,
      deck_a.b, deck_b.b};
  std::vector<Vec3F> pc = {floor_a.c, floor_b.c, riser_a.c, riser_b.c,
      deck_a.c, deck_b.c};
  std::vector<PhysicsMaterial> mats(6);
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 8);

  PhysicsStepConfig config;
  config.max_substep_move = 1.15f * units;
  config.skin = 0.06f * units;
  config.floor_normal_y = 0.45f;

  Vec3F start(-76.5f, floor_y + radius, step_z - 0.3f);
  Vec3F wish(0.0f, 0.0f, 3.07990003f);
  TEST_CHECK_(floor_y + step_h > start.y,
      "The step must reach above the sphere's center for this to be a wall "
      "at all, step_top=%f center=%f", floor_y + step_h, start.y);

  PhysicsManifold manifold;
  Vec3F at = start;
  bool saw_wall = false;
  const Si32 kFrames = 30;
  for (Si32 i = 0; i < kFrames; ++i) {
    SphereStepResult r = StepSphereBody(soup, &manifold, &at, radius, wish, dt,
        config);
    if (r.support.has_wall) {
      saw_wall = true;
    }
  }
  TEST_CHECK_(saw_wall,
      "A riser reaching above the sphere's center must still be reported "
      "as a wall");
  TEST_CHECK_(at.z < step_z - radius + config.skin,
      "The sphere must be held short of a step taller than itself instead "
      "of climbing it, z=%f step_z=%f radius=%f", at.z, step_z, radius);
  TEST_CHECK_(at.y < floor_y + radius + 0.1f * units,
      "The sphere must not creep up the riser it is stopped by, y=%f "
      "resting=%f", at.y, floor_y + radius);
}

void test_physics_drop_probe_conventions_at_a_seam() {
  // Locks what the two downward probes promise, because Hover Racer's hover
  // control depends on the difference. A road tile closes its edge with a
  // skirt hanging straight down, so the skirt grazes the fall path of a
  // sphere resting right next to the seam: the swept sphere touches it at
  // time zero, DropSphereBody cannot start its drop and hands the start
  // position back, and a caller that reads that as a landing measures its
  // own probe lift-off as a deficit (which pinned the craft's lift at zero
  // in the middle of the road). QuerySphereHeightBelow only looks at
  // floor-facing faces, so it still reports the road, and it reports the
  // surface height -- the radius has to go back on to get the height a
  // resting center sits at.
  const float radius = 0.276000023f;
  const float road_y = 6.09000349f;
  const float seam_z = 55.8258095f;
  const float up = 0.0702245f;

  const Vec3F faces_up(0.0f, 1.0f, 0.0f);
  const Vec3F away_z(0.0f, 0.0f, -1.0f);
  CollideSoupFixture fixture;
  fixture.AddFacing(
      Vec3F(-76.2699127f, 6.09000969f, 55.3730125f),
      Vec3F(-76.5137024f, 6.09001017f, 55.3653564f),
      Vec3F(-76.507164f, 6.09000349f, 55.8258095f), faces_up);
  fixture.AddFacing(
      Vec3F(-76.2699127f, 6.09000969f, 55.3730125f),
      Vec3F(-76.507164f, 6.09000349f, 55.8258095f),
      Vec3F(-76.2706528f, 6.09000158f, 55.8258934f), faces_up);
  // The skirt has to face the sphere for it to graze the fall path at all.
  fixture.AddFacing(
      Vec3F(-77.3672943f, 4.19974613f, 55.810318f),
      Vec3F(-76.4949417f, 6.09000349f, 55.8269501f),
      Vec3F(-76.2614441f, 6.09000015f, 55.8268738f), away_z);
  CollideSoup soup;
  fixture.Build(&soup);
  TEST_CHECK_(fixture.Ok(), "The fixture must describe a road and the skirt "
      "hanging off its edge:\n%s", fixture.Problems().c_str());

  const float max_drop = up + 40.0f * 0.140449f;
  const float backoff = 0.01f;
  Vec3F resting(-76.4620132f, 6.36600637f, 55.8240395f);
  Vec3F from(resting.x, resting.y + up, resting.z);
  TEST_CHECK_(std::fabs(resting.y - radius - road_y) < 1.0e-3f,
      "The sphere must start resting on the road, bottom=%f road_y=%f",
      resting.y - radius, road_y);

  SphereDropResult landed = DropSphereBody(soup, from, max_drop, radius,
      backoff);
  TEST_CHECK_(landed.kind == SphereDropResult::Kind::kBlockedAtStart,
      "Next to the seam the drop cannot start at all and must say so "
      "instead of handing back a place that looks like a landing, kind=%d "
      "fall=%f", static_cast<int>(landed.kind), landed.fall);
  TEST_CHECK_(std::fabs(landed.position.y - from.y) < 1.0e-6f,
      "A blocked drop must leave the sphere where it started, from.y=%f "
      "position.y=%f", from.y, landed.position.y);
  TEST_CHECK_(landed.normal.y < 0.45f,
      "The blocker beside the seam is the skirt, so the way out of it must "
      "not look like a floor -- this is how a caller tells 'a wall grazes "
      "me' from 'I am already standing on the ground', normal=(%f,%f,%f)",
      landed.normal.x, landed.normal.y, landed.normal.z);

  float surface = -1000.0f;
  bool found = QuerySphereHeightBelow(soup, from.x, from.z, from.y, max_drop,
      radius, 0.45f, &surface);
  TEST_CHECK_(found,
      "The floor-only probe must still find the road the sphere rests on");
  TEST_CHECK_(std::fabs(surface - road_y) < 1.0e-3f,
      "The floor-only probe must report the road surface itself, with no "
      "safety margin shaved off a measurement nothing is placed at, "
      "surface=%f road_y=%f", surface, road_y);
  float err = resting.y - (surface + radius);
  TEST_CHECK_(std::fabs(err) < 1.0e-3f,
      "A sphere already resting on the road must measure as being exactly "
      "at its height, err=%f", err);

  // A measurement must not depend on where the probe was launched from,
  // which is exactly what a margin taken as a fraction of the path would
  // break: from four units up, a tenth of the fall was most of half a unit
  // of made-up height.
  Vec3F high(resting.x, resting.y + 4.0f, resting.z);
  float high_surface = -1000.0f;
  TEST_CHECK_(QuerySphereHeightBelow(soup, high.x, high.z, high.y, max_drop,
          radius, 0.45f, &high_surface),
      "The floor-only probe must find the road from well above it too");
  TEST_CHECK_(std::fabs(high_surface - road_y) < 1.0e-3f,
      "The reported height must not depend on how far above the probe "
      "started, from_y=%f surface=%f road_y=%f", high.y, high_surface,
      road_y);

  // Away from the seam both probes agree, which is what makes the stall
  // above a property of the grazing face and not of the fixture.
  Vec3F clear(-76.4f, road_y + radius, 55.5f);
  Vec3F clear_from(clear.x, clear.y + up, clear.z);
  SphereDropResult clear_landed = DropSphereBody(soup, clear_from, max_drop,
      radius, backoff);
  TEST_CHECK_(clear_landed.kind == SphereDropResult::Kind::kLanded,
      "Away from the seam the drop must land, kind=%d",
      static_cast<int>(clear_landed.kind));
  TEST_CHECK_(clear_landed.normal.y > 0.9f,
      "A landing on the road must report the road as what stopped it, "
      "normal=(%f,%f,%f)", clear_landed.normal.x, clear_landed.normal.y,
      clear_landed.normal.z);
  float clear_surface = -1000.0f;
  TEST_CHECK_(QuerySphereHeightBelow(soup, clear_from.x, clear_from.z,
          clear_from.y, max_drop, radius, 0.45f, &clear_surface),
      "The floor-only probe must find the road away from the seam too");
  float clear_gap = clear_landed.position.y - (clear_surface + radius);
  TEST_CHECK_(std::fabs(clear_gap - backoff) < 0.002f,
      "Away from the seam a landing must sit exactly the asked-for margin "
      "above what the floor probe reports, gap=%f backoff=%f", clear_gap,
      backoff);

  // Nothing below is its own answer, distinct from both a landing and a
  // blocked start.
  Vec3F over_edge(-76.4f, road_y + radius, 56.5f);
  SphereDropResult fell = DropSphereBody(soup, over_edge, 5.0f, radius,
      backoff);
  TEST_CHECK_(fell.kind == SphereDropResult::Kind::kNothingBelow,
      "With nothing below the drop must say so, kind=%d",
      static_cast<int>(fell.kind));
  TEST_CHECK_(std::fabs(fell.fall - 5.0f) < 1.0e-3f,
      "With nothing below, the drop must take the whole max_drop, "
      "fall=%f max_drop=%f", fell.fall, 5.0f);
  float none = -1000.0f;
  TEST_CHECK_(!QuerySphereHeightBelow(soup, over_edge.x, over_edge.z,
          over_edge.y, 5.0f, radius, 0.45f, &none),
      "With nothing below, the floor-only probe must report a miss instead "
      "of a height, reported=%f", none);
}

void test_physics_sweep_backoff_is_a_distance_not_a_fraction() {
  // The margin a sweep keeps off what it hits guards against float slop
  // around the contact, and that slop does not grow with how far the sweep
  // happened to travel. While the margin was a tenth of the path, the same
  // wall stopped a long sweep a whole unit early and a short one a hair
  // early, so a camera boom drawn far back hung visibly short of the wall
  // and a probe launched from high above read the ground as high above too.
  const float radius = 0.5f;
  const float backoff = 0.05f;
  const float wall_x = 0.0f;

  CollisionTriangle wall_a = MakeTri(Vec3F(0.0f, -5.0f, -5.0f),
      Vec3F(0.0f, 5.0f, 5.0f), Vec3F(0.0f, 5.0f, -5.0f));
  CollisionTriangle wall_b = MakeTri(Vec3F(0.0f, -5.0f, -5.0f),
      Vec3F(0.0f, -5.0f, 5.0f), Vec3F(0.0f, 5.0f, 5.0f));
  Vec3F wall_n = wall_a.n / Length(wall_a.n);
  TEST_CHECK_(wall_n.x < -0.99f,
      "The wall must face the oncoming sphere, n=(%f,%f,%f)", wall_n.x,
      wall_n.y, wall_n.z);

  std::vector<Vec3F> pa = {wall_a.a, wall_b.a};
  std::vector<Vec3F> pb = {wall_a.b, wall_b.b};
  std::vector<Vec3F> pc = {wall_a.c, wall_b.c};
  std::vector<PhysicsMaterial> mats(2);
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 8);

  Vec3F target(2.0f, 0.0f, 0.0f);
  float expected_x = wall_x - radius - backoff;

  SweepSphereResult far_hit = SweepSphere(soup, Vec3F(-20.0f, 0.0f, 0.0f),
      target, radius, backoff);
  SweepSphereResult near_hit = SweepSphere(soup, Vec3F(-0.8f, 0.0f, 0.0f),
      target, radius, backoff);
  TEST_CHECK_(far_hit.kind == SweepSphereResult::Kind::kHit,
      "A sweep into the wall must report a hit, kind=%d",
      static_cast<int>(far_hit.kind));
  TEST_CHECK_(near_hit.kind == SweepSphereResult::Kind::kHit,
      "A short sweep into the wall must report a hit too, kind=%d",
      static_cast<int>(near_hit.kind));
  TEST_CHECK_(std::fabs(far_hit.position.x - expected_x) < 0.002f,
      "A sweep of twenty units must stop the asked-for margin short of the "
      "wall, x=%f expected=%f", far_hit.position.x, expected_x);
  TEST_CHECK_(std::fabs(near_hit.position.x - expected_x) < 0.002f,
      "A sweep of less than a unit must stop at the very same place, x=%f "
      "expected=%f", near_hit.position.x, expected_x);
  TEST_CHECK_(std::fabs(far_hit.position.x - near_hit.position.x) < 0.001f,
      "Distance traveled must not change where a sweep stops, far=%f "
      "near=%f", far_hit.position.x, near_hit.position.x);
  TEST_CHECK_(far_hit.normal.x < -0.99f,
      "A hit must report the way out of what was hit, normal=(%f,%f,%f)",
      far_hit.normal.x, far_hit.normal.y, far_hit.normal.z);

  SweepSphereResult exact = SweepSphere(soup, Vec3F(-20.0f, 0.0f, 0.0f),
      target, radius, 0.0f);
  TEST_CHECK_(std::fabs(exact.position.x - (wall_x - radius)) < 0.002f,
      "A zero margin must stop exactly at the touch, x=%f expected=%f",
      exact.position.x, wall_x - radius);

  SweepSphereResult clear = SweepSphere(soup, Vec3F(-20.0f, 0.0f, 0.0f),
      Vec3F(-10.0f, 0.0f, 0.0f), radius, backoff);
  TEST_CHECK_(clear.kind == SweepSphereResult::Kind::kClear,
      "A sweep that reaches its target must say the way was clear, kind=%d",
      static_cast<int>(clear.kind));
  TEST_CHECK_(std::fabs(clear.position.x + 10.0f) < 1.0e-5f,
      "A clear sweep must arrive at its target, x=%f", clear.position.x);
  TEST_CHECK_(LengthSquared(clear.normal) < 1.0e-10f,
      "A clear sweep has nothing to report a normal for, normal=(%f,%f,%f)",
      clear.normal.x, clear.normal.y, clear.normal.z);

  // Already touching is its own outcome: the position coming back unchanged
  // must not be readable as "the way ahead is blocked", which is the trap
  // the drop probe fell into at a road seam.
  SweepSphereResult stuck = SweepSphere(soup, Vec3F(-0.45f, 0.0f, 0.0f),
      target, radius, backoff);
  TEST_CHECK_(stuck.kind == SweepSphereResult::Kind::kStartOverlap,
      "A sweep that starts inside the wall must say so, kind=%d time=%f",
      static_cast<int>(stuck.kind), stuck.time);
  TEST_CHECK_(std::fabs(stuck.position.x + 0.45f) < 1.0e-6f,
      "A sweep blocked at the start must not move, x=%f", stuck.position.x);
}

void test_physics_step_report_merges_substeps() {
  // A Step can be cut into several substeps, and the report has to describe
  // the whole step. While it was simply the last substep's result, anything
  // that happened in an earlier one -- a floor touched, a wall hit, a
  // correction applied -- vanished, and a game that asks "what am I standing
  // on" once per frame saw a body that touched nothing.
  SphereStepResult early;
  early.contact_count = 3;
  early.outcome = ContactSolveOutcome::kDegenerate;
  early.position_correction = Vec3F(0.1f, 0.2f, 0.0f);
  early.velocity = Vec3F(9.0f, 0.0f, 0.0f);
  early.support.has_floor = true;
  early.support.floor_normal = Vec3F(0.0f, 1.0f, 0.0f);
  early.support.floor_material.grip = 0.25f;
  early.support.has_wall = true;
  early.support.floor_distance = 5.0f;
  early.support.has_floor_below = true;

  SphereStepResult late;
  late.contact_count = 1;
  late.outcome = ContactSolveOutcome::kFace;
  late.position_correction = Vec3F(0.0f, 0.3f, 0.0f);
  late.velocity = Vec3F(1.0f, 0.0f, 0.0f);
  late.support.has_ceiling = true;
  late.support.has_floor_below = true;
  late.support.floor_distance = 0.5f;

  SphereStepResult merged = MergeSphereStepResults(early, late);
  TEST_CHECK_(merged.support.has_floor,
      "A floor touched in an earlier substep must survive into the report");
  TEST_CHECK_(merged.support.floor_material.grip == 0.25f,
      "The surviving floor must bring its material along, grip=%f",
      merged.support.floor_material.grip);
  TEST_CHECK_(merged.support.has_wall && merged.support.has_ceiling,
      "Wall and ceiling flags must be the union of the substeps, wall=%d "
      "ceiling=%d", merged.support.has_wall ? 1 : 0,
      merged.support.has_ceiling ? 1 : 0);
  TEST_CHECK_(merged.contact_count == 3,
      "The busiest substep sets the contact count, got %d",
      merged.contact_count);
  TEST_CHECK_(merged.outcome == ContactSolveOutcome::kDegenerate,
      "The worst solver outcome must be the one reported, got %d",
      static_cast<int>(merged.outcome));
  TEST_CHECK_(std::fabs(merged.position_correction.y - 0.5f) < 1.0e-6f,
      "Corrections are cumulative and must add up, y=%f",
      merged.position_correction.y);
  TEST_CHECK_(std::fabs(merged.velocity.x - 1.0f) < 1.0e-6f,
      "The velocity is a state, so the last substep's is the step's, x=%f",
      merged.velocity.x);
  TEST_CHECK_(std::fabs(merged.support.floor_distance - 0.5f) < 1.0e-6f,
      "The measured height is about where the body ended up, so the last "
      "substep's reading wins, got %f", merged.support.floor_distance);

  SphereStepResult over_budget;
  over_budget.substep_budget_exhausted = true;
  TEST_CHECK_(MergeSphereStepResults(over_budget, late)
          .substep_budget_exhausted,
      "An exhausted substep budget must not be forgotten by the merge");

  // Same thing through the world, where the substeps actually happen: a
  // plate that ends halfway through the motion is a floor the body only
  // touches during the first substeps.
  CollisionTriangle plate_a = MakeTri(Vec3F(-5.0f, 0.0f, -5.0f),
      Vec3F(-5.0f, 0.0f, 5.0f), Vec3F(0.0f, 0.0f, 5.0f));
  CollisionTriangle plate_b = MakeTri(Vec3F(-5.0f, 0.0f, -5.0f),
      Vec3F(0.0f, 0.0f, 5.0f), Vec3F(0.0f, 0.0f, -5.0f));
  TEST_CHECK_(plate_a.n.y > 0.99f && plate_b.n.y > 0.99f,
      "test setup: the plate must face up, a=%f b=%f", plate_a.n.y,
      plate_b.n.y);
  std::vector<Vec3F> pa = {plate_a.a, plate_b.a};
  std::vector<Vec3F> pb = {plate_a.b, plate_b.b};
  std::vector<Vec3F> pc = {plate_a.c, plate_b.c};
  std::vector<PhysicsMaterial> mats(2);

  const float radius = 0.5f;
  const float dt = 1.0f / 60.0f;
  PhysicsStepConfig config;
  config.max_substep_move = 0.2f;
  config.max_substeps = 16;
  PhysicsWorld world;
  world.SetConfig(config);
  world.SetStaticMesh(pa, pb, pc, mats, 8);
  PhysicsBodyId id = world.AddSphere(Vec3F(-0.6f, radius, 0.0f), radius);
  world.SetWishVelocity(id, Vec3F(72.0f, 0.0f, 0.0f));
  world.Step(dt);
  TEST_CHECK_(world.Position(id).x > 0.2f,
      "The body must have run off the end of the plate for this to be "
      "about several substeps, x=%f", world.Position(id).x);
  TEST_CHECK_(world.Support(id).has_floor,
      "The plate touched during the first substeps must be in the step's "
      "report even though the body ended up past its edge");
  TEST_CHECK_(!world.SubstepBudgetExhausted(id),
      "With sixteen substeps allowed this motion must fit in the budget");

  // The last substep on its own sees nothing, which is what makes the check
  // above a statement about merging and not about the geometry.
  Vec3F past_edge = world.Position(id);
  SphereStepResult alone = StepSphereBody(world.Soup(), nullptr, &past_edge,
      radius, Vec3F(72.0f, 0.0f, 0.0f), dt / 7.0f, config);
  TEST_CHECK_(!alone.support.has_floor,
      "test setup: past the edge a lone substep must find no floor");

  // The cap on substeps is the anti-tunneling promise: a step that needs
  // more of them than allowed is moving further per substep than
  // max_substep_move, and that has to be said out loud.
  PhysicsStepConfig tight = config;
  tight.max_substeps = 2;
  PhysicsWorld tight_world;
  tight_world.SetConfig(tight);
  tight_world.SetStaticMesh(pa, pb, pc, mats, 8);
  PhysicsBodyId tight_id = tight_world.AddSphere(Vec3F(-4.0f, radius, 0.0f),
      radius);
  tight_world.SetWishVelocity(tight_id, Vec3F(72.0f, 0.0f, 0.0f));
  tight_world.Step(dt);
  TEST_CHECK_(tight_world.SubstepBudgetExhausted(tight_id),
      "A motion needing seven substeps with two allowed must report the "
      "budget as exhausted");
  tight_world.SetWishVelocity(tight_id, Vec3F(1.0f, 0.0f, 0.0f));
  tight_world.Step(dt);
  TEST_CHECK_(!tight_world.SubstepBudgetExhausted(tight_id),
      "A slow motion must clear the flag again");
}

void test_physics_body_separation_stays_out_of_the_mesh() {
  // Two bodies squeezed together push each other apart in XZ, and one of
  // them can be against a wall. While that push was written straight into
  // the position, a big enough shove carried the center clear through a
  // face, and the push-out afterwards helpfully settled the body on the far
  // side of the wall -- the same "write the position and hope" that the
  // solver exists to avoid, and that Hover Racer already paid for once.
  CollisionTriangle floor_a = MakeTri(Vec3F(-5.0f, 0.0f, -5.0f),
      Vec3F(-5.0f, 0.0f, 5.0f), Vec3F(5.0f, 0.0f, 5.0f));
  CollisionTriangle floor_b = MakeTri(Vec3F(-5.0f, 0.0f, -5.0f),
      Vec3F(5.0f, 0.0f, 5.0f), Vec3F(5.0f, 0.0f, -5.0f));
  const float wall_x = 2.0f;
  CollisionTriangle wall_a = MakeTri(Vec3F(wall_x, 0.0f, -5.0f),
      Vec3F(wall_x, 3.0f, 5.0f), Vec3F(wall_x, 3.0f, -5.0f));
  CollisionTriangle wall_b = MakeTri(Vec3F(wall_x, 0.0f, -5.0f),
      Vec3F(wall_x, 0.0f, 5.0f), Vec3F(wall_x, 3.0f, 5.0f));
  TEST_CHECK_(floor_a.n.y > 0.99f && floor_b.n.y > 0.99f,
      "test setup: the floor must face up, a=%f b=%f", floor_a.n.y,
      floor_b.n.y);
  Vec3F wall_n = wall_a.n / Length(wall_a.n);
  TEST_CHECK_(wall_n.x < -0.99f,
      "test setup: the wall must face the bodies, n=(%f,%f,%f)", wall_n.x,
      wall_n.y, wall_n.z);

  std::vector<Vec3F> pa = {floor_a.a, floor_b.a, wall_a.a, wall_b.a};
  std::vector<Vec3F> pb = {floor_a.b, floor_b.b, wall_a.b, wall_b.b};
  std::vector<Vec3F> pc = {floor_a.c, floor_b.c, wall_a.c, wall_b.c};
  std::vector<PhysicsMaterial> mats(4);

  const float radius = 0.5f;
  const float dt = 1.0f / 60.0f;
  PhysicsStepConfig config;
  PhysicsWorld world;
  world.SetConfig(config);
  world.SetStaticMesh(pa, pb, pc, mats, 8);

  // Nearly coincident, and all of the separation falls on the body next to
  // the wall: the shove is then more than a radius, which is exactly when
  // writing the position walks the center to the other side of the face.
  PhysicsBodyId pressed = world.AddSphere(Vec3F(1.2f, radius, 0.0f), radius);
  PhysicsBodyId holder = world.AddSphere(Vec3F(1.199f, radius, 0.0f),
      radius);
  world.SetPushWeight(pressed, 1.0f);
  world.SetPushWeight(holder, 0.0f);
  TEST_CHECK_(world.Position(pressed).x + radius < wall_x,
      "test setup: the pressed body must start clear of the wall, x=%f",
      world.Position(pressed).x);

  world.Step(dt);

  float x = world.Position(pressed).x;
  TEST_CHECK_(x + radius <= wall_x + config.skin,
      "The separated body must stay on this side of the wall, x=%f "
      "wall=%f radius=%f", x, wall_x, radius);
  TEST_CHECK_(x > 1.2f + 0.05f,
      "The separation must still have happened, otherwise the check above "
      "passes for the wrong reason, x=%f", x);
  TEST_CHECK_(std::fabs(world.Position(holder).x - 1.199f) < 1.0e-4f,
      "A body with no push weight must not be moved by the separation, "
      "x=%f", world.Position(holder).x);
  TEST_CHECK_(std::fabs(world.Position(pressed).y - radius) < config.skin,
      "Separating bodies must not lift them off the floor, y=%f", 
      world.Position(pressed).y);
}

void test_physics_world_query_conventions() {
  // The world's four queries are what a game reaches for, and every one of
  // them has a convention that is easy to get wrong in the caller and
  // impossible to see in a happy-path test: what a miss looks like, what
  // "the answer is where you already are" looks like, what happens exactly
  // at the end of the asked-for range, and how far short of geometry a
  // query stops. Hover Racer read three of these wrong in turn.
  const float radius = 0.5f;
  const float floor_y = 0.0f;
  const float wall_x = 2.0f;
  const Vec3F up(0.0f, 1.0f, 0.0f);
  const Vec3F towards_minus_x(-1.0f, 0.0f, 0.0f);

  CollideSoupFixture fixture;
  fixture.AddFacing(Vec3F(-5.0f, floor_y, -5.0f), Vec3F(-5.0f, floor_y, 5.0f),
      Vec3F(5.0f, floor_y, 5.0f), up);
  fixture.AddFacing(Vec3F(-5.0f, floor_y, -5.0f), Vec3F(5.0f, floor_y, 5.0f),
      Vec3F(5.0f, floor_y, -5.0f), up);
  fixture.AddFacing(Vec3F(wall_x, 0.0f, -5.0f), Vec3F(wall_x, 4.0f, 5.0f),
      Vec3F(wall_x, 4.0f, -5.0f), towards_minus_x);
  fixture.AddFacing(Vec3F(wall_x, 0.0f, -5.0f), Vec3F(wall_x, 0.0f, 5.0f),
      Vec3F(wall_x, 4.0f, 5.0f), towards_minus_x);

  PhysicsStepConfig config;
  config.skin = 0.04f;
  PhysicsWorld world;
  world.SetConfig(config);
  {
    CollideSoup soup;
    fixture.Build(&soup);
    TEST_CHECK_(fixture.Ok(), "The fixture must be a floor and a wall:\n%s",
        fixture.Problems().c_str());
    std::vector<Vec3F> pa, pb, pc;
    std::vector<PhysicsMaterial> mats;
    for (Si32 i = 0; i < soup.TriangleCount(); ++i) {
      pa.push_back(soup.Triangle(i).a);
      pb.push_back(soup.Triangle(i).b);
      pc.push_back(soup.Triangle(i).c);
      mats.push_back(soup.Material(i));
    }
    world.SetStaticMesh(pa, pb, pc, mats, 8);
  }

  // A sweep keeps the world's own skin off what it hits, and says which of
  // the three things happened rather than leaving the caller to guess from a
  // position.
  const Vec3F lane(0.0f, floor_y + radius + 1.0f, 0.0f);
  SweepSphereResult hit = world.SweepSphereQuery(lane,
      Vec3F(wall_x + 1.0f, lane.y, lane.z), radius);
  TEST_CHECK_(hit.kind == SweepSphereResult::Kind::kHit,
      "A sweep into the wall must report a hit, kind=%d",
      static_cast<int>(hit.kind));
  const float expected_x = wall_x - radius - config.skin;
  TEST_CHECK_(std::fabs(hit.position.x - expected_x) < 0.002f,
      "A sweep must stop the world's skin short of the wall, x=%f "
      "expected=%f skin=%f", hit.position.x, expected_x, config.skin);
  SweepSphereResult clear = world.SweepSphereQuery(lane,
      Vec3F(1.0f, lane.y, lane.z), radius);
  TEST_CHECK_(clear.kind == SweepSphereResult::Kind::kClear
          && std::fabs(clear.position.x - 1.0f) < 1.0e-5f,
      "A sweep with room to spare must arrive where it was sent, kind=%d "
      "x=%f", static_cast<int>(clear.kind), clear.position.x);
  SweepSphereResult inside = world.SweepSphereQuery(
      Vec3F(wall_x - 0.4f * radius, lane.y, lane.z),
      Vec3F(wall_x + 1.0f, lane.y, lane.z), radius);
  TEST_CHECK_(inside.kind == SweepSphereResult::Kind::kStartOverlap,
      "A sweep that starts inside the wall must say so and not look like "
      "a hit a hair ahead, kind=%d", static_cast<int>(inside.kind));

  // The drop probe: landing, nothing below, and blocked before it began are
  // three different answers.
  SphereDropResult landed = world.DropSphereQuery(
      Vec3F(0.0f, floor_y + radius + 2.0f, 0.0f), 10.0f, radius);
  TEST_CHECK_(landed.kind == SphereDropResult::Kind::kLanded,
      "A drop onto the floor must land, kind=%d",
      static_cast<int>(landed.kind));
  TEST_CHECK_(std::fabs(landed.position.y - (floor_y + radius + config.skin))
          < 0.003f,
      "A landing must sit the world's skin above the floor, y=%f "
      "expected=%f", landed.position.y, floor_y + radius + config.skin);
  SphereDropResult nothing = world.DropSphereQuery(
      Vec3F(0.0f, floor_y + 8.0f, 0.0f), 2.0f, radius);
  TEST_CHECK_(nothing.kind == SphereDropResult::Kind::kNothingBelow,
      "A drop that runs out of range must report a miss and not a landing "
      "at the bottom of the range, kind=%d",
      static_cast<int>(nothing.kind));
  TEST_CHECK_(std::fabs(nothing.fall - 2.0f) < 1.0e-3f,
      "A drop with nothing below must take the whole range, fall=%f",
      nothing.fall);
  SphereDropResult blocked = world.DropSphereQuery(
      Vec3F(wall_x - 0.4f * radius, floor_y + 2.0f, 0.0f), 10.0f, radius);
  TEST_CHECK_(blocked.kind == SphereDropResult::Kind::kBlockedAtStart,
      "A drop that starts inside the wall must say it never began, kind=%d",
      static_cast<int>(blocked.kind));

  // The height probe: max_drop is how far the center may fall, so a floor
  // `d` below a resting sphere is exactly d + radius of range away. The far
  // end of the range is inclusive, and the answer does not depend on how far
  // away the question was asked from -- the last of these was wrong while
  // the margin was a fraction of the path.
  const float from_y = floor_y + 3.0f;
  const float exact_range = from_y - floor_y - radius;
  float height = -1000.0f;
  TEST_CHECK_(world.HeightBelowQuery(0.0f, 0.0f, from_y, exact_range, radius,
          &height),
      "A floor exactly at the end of the range must be found, range=%f",
      exact_range);
  TEST_CHECK_(std::fabs(height - floor_y) < 1.0e-3f,
      "The height reported must be the floor itself, height=%f floor=%f",
      height, floor_y);
  float short_height = -1000.0f;
  TEST_CHECK_(!world.HeightBelowQuery(0.0f, 0.0f, from_y,
          exact_range - 0.1f, radius, &short_height),
      "A floor just past the end of the range must be a miss, reported=%f",
      short_height);
  TEST_CHECK_(short_height == -1000.0f,
      "A miss must leave the caller's number alone rather than writing "
      "something into it, got %f", short_height);
  float far_height = -1000.0f;
  TEST_CHECK_(world.HeightBelowQuery(0.0f, 0.0f, floor_y + 30.0f, 60.0f,
          radius, &far_height),
      "The floor must be found from far above as well");
  TEST_CHECK_(std::fabs(far_height - height) < 1.0e-3f,
      "A measurement must not change with the distance it was taken from, "
      "near=%f far=%f", height, far_height);

  // Unsticking answers about the state it leaves the center in, not about
  // whether it had work to do: true is "free where it is now", and a sphere
  // that overlapped nothing is free without being moved.
  Vec3F free_center(0.0f, floor_y + radius + 1.0f, 0.0f);
  Vec3F untouched = free_center;
  TEST_CHECK_(world.UnstickQuery(&free_center, radius),
      "A sphere touching nothing is already free and must be reported so");
  TEST_CHECK_(Length(free_center - untouched) < 1.0e-6f,
      "A sphere that needed no unsticking must not be moved, moved by %f",
      Length(free_center - untouched));
  Vec3F buried(0.0f, floor_y + 0.2f * radius, 0.0f);
  TEST_CHECK_(world.UnstickQuery(&buried, radius),
      "A sphere sunk into the floor must be reported as free afterwards");
  TEST_CHECK_(buried.y >= floor_y + radius - 1.0e-3f,
      "An unstuck sphere must end up out of the floor, y=%f floor+r=%f",
      buried.y, floor_y + radius);
  TEST_CHECK_(std::fabs(buried.x) < 1.0e-4f && std::fabs(buried.z) < 1.0e-4f,
      "Unsticking must push straight out of the face and not slide the "
      "sphere along it, x=%f z=%f", buried.x, buried.z);
}

void test_physics_broad_phase_offers_every_overlapping_triangle() {
  // The broad-phase grid is allowed to offer too much and never too little:
  // the narrow phase filters candidates, so a triangle that is not offered
  // simply does not exist as far as collision is concerned. That failure is
  // invisible in ordinary play and was found once already as "a big
  // triangle is not found", when a face spanning many cells was only
  // registered in the cells its corners fell into. Two checks here: a
  // random mesh against a full scan, and one long triangle asked for from
  // every cell it crosses.
  Ui64 seed = 0x5eed1234u;
  auto next = [&seed]() {
    seed = seed * 6364136223846793005ull + 1442695040888963407ull;
    return static_cast<float>((seed >> 33) & 0xffffff)
        / static_cast<float>(0x1000000);
  };
  auto span = [&next](float lo, float hi) {
    return lo + (hi - lo) * next();
  };

  std::vector<Vec3F> pa, pb, pc;
  for (Si32 i = 0; i < 220; ++i) {
    // A mix of sizes on purpose: the small ones fit inside a cell, the big
    // ones cross many, and only the big ones can expose a grid that
    // registers a face by its corners.
    float reach = (i % 11 == 0) ? span(20.0f, 60.0f) : span(0.2f, 3.0f);
    Vec3F a(span(-40.0f, 40.0f), span(-10.0f, 10.0f), span(-40.0f, 40.0f));
    Vec3F b = a + Vec3F(span(-reach, reach), span(-reach, reach),
        span(-reach, reach));
    Vec3F c = a + Vec3F(span(-reach, reach), span(-reach, reach),
        span(-reach, reach));
    CollisionTriangle probe;
    if (!probe.Set(a, b, c)) {
      continue;
    }
    pa.push_back(a);
    pb.push_back(b);
    pc.push_back(c);
  }
  std::vector<PhysicsMaterial> mats(pa.size());
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 16);
  TEST_CHECK_(soup.TriangleCount() > 180,
      "test setup: the random mesh must have kept its faces, got %d",
      soup.TriangleCount());

  Si32 boxes_with_hits = 0;
  for (Si32 q = 0; q < 400; ++q) {
    float half = span(0.05f, 4.0f);
    Vec3F at(span(-45.0f, 45.0f), span(-12.0f, 12.0f), span(-45.0f, 45.0f));
    Bound3F box(at.x - half, at.x + half, at.y - half, at.y + half,
        at.z - half, at.z + half);

    std::vector<bool> offered(static_cast<size_t>(soup.TriangleCount()),
        false);
    Si32 offers = 0;
    soup.ForEachNear(box, [&offered, &offers](Si32 index) {
      offered[static_cast<size_t>(index)] = true;
      ++offers;
    });
    TEST_CHECK_(offers == static_cast<Si32>(std::count(offered.begin(),
            offered.end(), true)),
        "The grid must offer each triangle once per query, offers=%d "
        "distinct=%d", offers,
        static_cast<Si32>(std::count(offered.begin(), offered.end(), true)));

    for (Si32 i = 0; i < soup.TriangleCount(); ++i) {
      const CollisionTriangle &tri = soup.Triangle(i);
      Bound3F tri_box(
          std::min(tri.a.x, std::min(tri.b.x, tri.c.x)),
          std::max(tri.a.x, std::max(tri.b.x, tri.c.x)),
          std::min(tri.a.y, std::min(tri.b.y, tri.c.y)),
          std::max(tri.a.y, std::max(tri.b.y, tri.c.y)),
          std::min(tri.a.z, std::min(tri.b.z, tri.c.z)),
          std::max(tri.a.z, std::max(tri.b.z, tri.c.z)));
      bool overlaps = tri_box.max_x >= box.min_x && tri_box.min_x <= box.max_x
          && tri_box.max_y >= box.min_y && tri_box.min_y <= box.max_y
          && tri_box.max_z >= box.min_z && tri_box.min_z <= box.max_z;
      if (!overlaps) {
        continue;
      }
      ++boxes_with_hits;
      if (!TEST_CHECK_(offered[static_cast<size_t>(i)],
          "The grid did not offer triangle %d, whose box "
          "[%f..%f][%f..%f][%f..%f] overlaps the query "
          "[%f..%f][%f..%f][%f..%f]", i, tri_box.min_x, tri_box.max_x,
          tri_box.min_y, tri_box.max_y, tri_box.min_z, tri_box.max_z,
          box.min_x, box.max_x, box.min_y, box.max_y, box.min_z,
          box.max_z)) {
        return;
      }
    }
  }
  TEST_CHECK_(boxes_with_hits > 100,
      "test setup: the random queries must have found something to compare, "
      "hits=%d", boxes_with_hits);

  // One face across the whole mesh, asked for from a small box walking
  // along it: this is the shape of the bug that was already fixed once, and
  // it must stay fixed cell by cell rather than on average.
  const float far_end = 90.0f;
  std::vector<Vec3F> la = {Vec3F(-far_end, 0.0f, -far_end),
      Vec3F(-5.0f, -1.0f, -5.0f)};
  std::vector<Vec3F> lb = {Vec3F(far_end, 0.0f, far_end),
      Vec3F(5.0f, -1.0f, -5.0f)};
  std::vector<Vec3F> lc = {Vec3F(-far_end, 2.0f, -far_end + 1.0f),
      Vec3F(5.0f, -1.0f, 5.0f)};
  std::vector<PhysicsMaterial> long_mats(2);
  CollideSoup long_soup;
  long_soup.Build(la, lb, lc, long_mats, 64);
  TEST_CHECK_(long_soup.TriangleCount() == 2,
      "test setup: the long mesh must keep both faces, got %d",
      long_soup.TriangleCount());

  const Si32 kSteps = 200;
  Si32 asked = 0;
  for (Si32 i = 0; i <= kSteps; ++i) {
    float t = static_cast<float>(i) / static_cast<float>(kSteps);
    float x = -far_end + 2.0f * far_end * t;
    float z = x;
    Bound3F box(x - 0.05f, x + 0.05f, -1.0f, 3.0f, z - 0.05f, z + 0.05f);
    bool found = false;
    long_soup.ForEachNear(box, [&found](Si32 index) {
      if (index == 0) {
        found = true;
      }
    });
    ++asked;
    if (!TEST_CHECK_(found,
        "A face crossing the whole mesh must be offered from every cell it "
        "passes through, and it was not at (%f, %f)", x, z)) {
      return;
    }
  }
  TEST_CHECK_(asked == kSteps + 1,
      "test setup: every step along the face must have been asked, "
      "asked=%d", asked);
}

void test_physics_fixture_builder_catches_bad_geometry() {
  // Every physics fixture in this file is transcribed geometry, and the two
  // ways transcription goes wrong are silent: a degenerate face is dropped
  // by the mesh builder without a word, and a face wound the other way
  // still builds -- it just points the wrong direction, which comes out
  // several layers later as a sphere falling through a floor. The builder
  // exists to turn both into a message that names the face.
  const Vec3F up(0.0f, 1.0f, 0.0f);

  CollideSoupFixture good;
  Si32 first = good.AddFacing(Vec3F(-1.0f, 0.0f, -1.0f),
      Vec3F(-1.0f, 0.0f, 1.0f), Vec3F(1.0f, 0.0f, 1.0f), up);
  Si32 second = good.AddFacing(Vec3F(-1.0f, 0.0f, -1.0f),
      Vec3F(1.0f, 0.0f, 1.0f), Vec3F(1.0f, 0.0f, -1.0f), up);
  TEST_CHECK_(first == 0 && second == 1,
      "Faces must come back numbered in the order they were added, got %d "
      "and %d", first, second);
  CollideSoup soup;
  good.Build(&soup);
  TEST_CHECK_(good.Ok(), "A correct fixture must report nothing wrong:\n%s",
      good.Problems().c_str());
  TEST_CHECK_(soup.TriangleCount() == 2,
      "The built mesh must keep both faces, got %d", soup.TriangleCount());
  TEST_CHECK_(good.NormalLines().size() == 2,
      "There must be one normal line per face, got %d",
      static_cast<int>(good.NormalLines().size()));

  CollideSoupFixture flipped;
  flipped.AddFacing(Vec3F(-1.0f, 0.0f, -1.0f), Vec3F(1.0f, 0.0f, 1.0f),
      Vec3F(-1.0f, 0.0f, 1.0f), up);
  TEST_CHECK_(!flipped.Ok(),
      "A face wound the other way must not pass as facing up");
  TEST_CHECK_(flipped.Problems().find("order of its vertices")
          != std::string::npos,
      "The complaint must say what to look at, got: %s",
      flipped.Problems().c_str());

  CollideSoupFixture degenerate;
  Si32 bad = degenerate.Add(Vec3F(0.0f, 0.0f, 0.0f), Vec3F(1.0f, 0.0f, 0.0f),
      Vec3F(2.0f, 0.0f, 0.0f));
  TEST_CHECK_(bad < 0,
      "A degenerate face must not be handed back as a usable one, got %d",
      bad);
  TEST_CHECK_(!degenerate.Ok() &&
          degenerate.Problems().find("degenerate") != std::string::npos,
      "A degenerate face must be reported, got: %s",
      degenerate.Problems().c_str());
  TEST_CHECK_(degenerate.Count() == 0,
      "A degenerate face must not be kept, count=%d", degenerate.Count());
}

void test_physics_support_measures_height_above_floor() {
  // A hover control has to start braking while still in the air, so it needs
  // the distance to the ground and not just "am I touching". Before the
  // support carried that measurement, Hover Racer probed on its own, drifted
  // away from the engine's own reading and froze mid-road on it; the point of
  // these checks is that the engine's own answer is exact, opt-in, and fresh
  // after a teleport.
  CollisionTriangle floor_a = MakeTri(
      Vec3F(-5.0f, 0.0f, -5.0f), Vec3F(-5.0f, 0.0f, 5.0f),
      Vec3F(5.0f, 0.0f, 5.0f));
  CollisionTriangle floor_b = MakeTri(
      Vec3F(-5.0f, 0.0f, -5.0f), Vec3F(5.0f, 0.0f, 5.0f),
      Vec3F(5.0f, 0.0f, -5.0f));
  TEST_CHECK_(floor_a.n.y > 0.99f && floor_b.n.y > 0.99f,
      "test setup: both floor triangles must face up, a=%f b=%f",
      floor_a.n.y, floor_b.n.y);
  std::vector<Vec3F> pa = {floor_a.a, floor_b.a};
  std::vector<Vec3F> pb = {floor_a.b, floor_b.b};
  std::vector<Vec3F> pc = {floor_a.c, floor_b.c};
  std::vector<PhysicsMaterial> mats(2);
  CollideSoup soup;
  soup.Build(pa, pb, pc, mats, 8);

  const float radius = 0.276000023f;
  const float dt = 1.0f / 60.0f;
  const Vec3F still(0.0f, 0.0f, 0.0f);
  PhysicsStepConfig config;
  config.support_probe = 6.0f;

  Vec3F resting(0.0f, radius, 0.0f);
  SphereStepResult rest_step = StepSphereBody(soup, nullptr, &resting, radius,
      still, dt, config);
  TEST_CHECK_(rest_step.support.has_floor_below,
      "A sphere on the floor must find a floor below it");
  TEST_CHECK_(std::fabs(rest_step.support.floor_distance) < config.skin,
      "A resting sphere must measure as zero away from the floor, got %f "
      "skin=%f", rest_step.support.floor_distance, config.skin);

  // Airborne, and measured at two very different heights: the answer has to
  // be the real height both times. A margin proportional to the fall (which
  // is what the sweeps keep for placing bodies) would show up here as a
  // reading that grows wrong with distance.
  const float heights[2] = {0.5f, 5.0f};
  for (Si32 i = 0; i < 2; ++i) {
    Vec3F pos(0.0f, radius + heights[i], 0.0f);
    SphereStepResult step = StepSphereBody(soup, nullptr, &pos, radius,
        still, dt, config);
    TEST_CHECK_(step.support.has_floor_below,
        "The floor must be found from %f up", heights[i]);
    TEST_CHECK_(!step.support.has_floor,
        "Hanging %f above the floor is not resting on it", heights[i]);
    TEST_CHECK_(std::fabs(step.support.floor_distance - heights[i]) < 1.0e-3f,
        "The measured height must be the real one, measured=%f real=%f",
        step.support.floor_distance, heights[i]);
  }

  Vec3F too_high(0.0f, radius + config.support_probe + 1.0f, 0.0f);
  SphereStepResult high_step = StepSphereBody(soup, nullptr, &too_high,
      radius, still, dt, config);
  TEST_CHECK_(!high_step.support.has_floor_below,
      "A floor past support_probe must not be reported, distance=%f "
      "probe=%f", high_step.support.floor_distance, config.support_probe);

  Vec3F off_the_edge(20.0f, radius, 20.0f);
  SphereStepResult off_step = StepSphereBody(soup, nullptr, &off_the_edge,
      radius, still, dt, config);
  TEST_CHECK_(!off_step.support.has_floor_below,
      "With no floor under it at all the measurement must report nothing, "
      "distance=%f", off_step.support.floor_distance);

  // Opt-in: a caller with no use for the measurement must not pay for it.
  PhysicsStepConfig no_probe;
  no_probe.support_probe = 0.0f;
  Vec3F above(0.0f, radius + 1.0f, 0.0f);
  SphereStepResult no_probe_step = StepSphereBody(soup, nullptr, &above,
      radius, still, dt, no_probe);
  TEST_CHECK_(!no_probe_step.support.has_floor_below,
      "With support_probe at zero nothing may be measured, distance=%f",
      no_probe_step.support.floor_distance);

  // Through a real fall the measurement must track the height the geometry
  // says, every step, not just at the ends.
  PhysicsManifold manifold;
  Vec3F falling(0.0f, radius + 4.0f, 0.0f);
  float lift = 0.0f;
  float worst_error = 0.0f;
  bool ever_missed = false;
  for (Si32 i = 0; i < 120; ++i) {
    lift -= 9.8f * dt;
    SphereStepResult step = StepSphereBody(soup, &manifold, &falling, radius,
        Vec3F(0.0f, lift, 0.0f), dt, config);
    if (step.support.has_floor) {
      lift = 0.0f;
    }
    if (!step.support.has_floor_below) {
      ever_missed = true;
      continue;
    }
    float truth = falling.y - radius;
    float error = std::fabs(step.support.floor_distance - truth);
    if (error > worst_error) {
      worst_error = error;
    }
  }
  TEST_CHECK_(!ever_missed,
      "The floor must stay found for the whole fall from four units up");
  TEST_CHECK_(worst_error < 1.0e-3f,
      "The measurement must follow the true height through the fall, worst "
      "error=%f", worst_error);
  TEST_CHECK_(falling.y <= radius + config.skin,
      "The falling sphere must land, y=%f", falling.y);

  // A teleport is the one moment the last step's measurement is about the
  // wrong place, so the world refreshes it right there. A hover control
  // reads the support before it steps, and on the frame after a spawn that
  // would otherwise be an answer about where the body used to be.
  PhysicsWorld world;
  world.SetConfig(config);
  world.SetStaticMesh(pa, pb, pc, mats, 8);
  PhysicsBodyId id = world.AddSphere(Vec3F(0.0f, radius, 0.0f), radius);
  world.Teleport(id, Vec3F(0.0f, radius + 2.5f, 0.0f));
  TEST_CHECK_(world.Support(id).has_floor_below,
      "Right after a teleport the support must already know about the "
      "floor below the new place");
  TEST_CHECK_(std::fabs(world.Support(id).floor_distance - 2.5f) < 1.0e-3f,
      "The refreshed measurement must be about the new place, got %f "
      "expected 2.5", world.Support(id).floor_distance);
  world.Teleport(id, Vec3F(20.0f, radius, 20.0f));
  TEST_CHECK_(!world.Support(id).has_floor_below,
      "Teleported off the mesh, the support must stop claiming a floor "
      "below, distance=%f", world.Support(id).floor_distance);

  // The support has to be a value. While it was a reference into the vector
  // the bodies live in, registering another body could reallocate that
  // vector under a caller still holding the reference -- silent, and of the
  // kind that only shows up once the field grows.
  static_assert(std::is_same<decltype(world.Support(id)),
      SphereBodySupport>::value,
      "PhysicsWorld::Support must return a value, not a reference into the "
      "body vector");
  SphereBodySupport kept = world.Support(id);
  for (Si32 i = 0; i < 32; ++i) {
    world.AddSphere(Vec3F(100.0f + static_cast<float>(i), radius, 100.0f),
        radius);
  }
  TEST_CHECK_(kept.has_floor_below == world.Support(id).has_floor_below &&
          std::fabs(kept.floor_distance - world.Support(id).floor_distance)
              < 1.0e-6f,
      "A support taken before the field grew must still describe the same "
      "body");

  // Moving a body by hand is the one thing a solver cannot account for, so
  // the world counts it and a game can assert the count is zero on the
  // frames it did not mean to teleport. Hover Racer needed exactly this
  // check, and had to watch positions itself to get it.
  TEST_CHECK_(world.TeleportsSinceStep() == 2,
      "Both teleports must be counted, got %d", world.TeleportsSinceStep());
  world.Step(1.0f / 60.0f);
  TEST_CHECK_(world.TeleportsSinceStep() == 0,
      "A step must clear the teleport count, got %d",
      world.TeleportsSinceStep());
  Vec3F before_step = world.Position(id);
  world.SetWishVelocity(id, Vec3F(1.0f, 0.0f, 0.0f));
  world.Step(1.0f / 60.0f);
  TEST_CHECK_(world.TeleportsSinceStep() == 0,
      "Driving a body must not look like a teleport, got %d",
      world.TeleportsSinceStep());
  TEST_CHECK_(Length(world.Position(id) - before_step) > 1.0e-4f,
      "The driven body must have moved for that check to mean anything");
}
