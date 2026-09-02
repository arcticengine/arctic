// Math: quaternions, Transform3F, the rotation and projection matrices of
// Mat44F, the skeleton.
#define TEST_NO_MAIN
#include "test_helpers.h"

// ============================================================================
// Quaternion bug tests
// ============================================================================

// Bug: ToMat33F has sign errors in the w*x terms at positions [5] and [7].
//
// The correct rotation matrix from unit quaternion (x,y,z,w) is:
//   [5] = 2*y*z - 2*w*x
//   [7] = 2*y*z + 2*w*x
// But the code has the signs swapped:
//   [5] = 2*y*z + 2*w*x   (wrong)
//   [7] = 2*y*z - 2*w*x   (wrong)
//
// Test: 90-degree rotation around X should map (0,1,0) to (0,0,1).
// The bug flips the rotation direction, mapping (0,1,0) to (0,0,-1).
void test_quat_to_mat33f_sign() {
  float angle = static_cast<float>(kPi / 2.0);
  Vec3F axis(1.0f, 0.0f, 0.0f);
  QuaternionF q(axis, angle);

  Mat33F mat = q.ToMat33F();
  Vec3F v(0.0f, 1.0f, 0.0f);
  Vec3F result = mat * v;

  // 90-deg rotation around X: (0,1,0) -> (0,0,1)
  TEST_CHECK_(fabsf(result.x) < 0.001f,
      "Quat X-rot: result.x should be ~0, got %f", result.x);
  TEST_CHECK_(fabsf(result.y) < 0.001f,
      "Quat X-rot: result.y should be ~0, got %f", result.y);
  TEST_CHECK_(fabsf(result.z - 1.0f) < 0.001f,
      "Quat X-rot: result.z should be ~1, got %f", result.z);
}

// Same sign bug exists in ToPartialMatrix33F (copy of ToMat33F logic).
void test_quat_to_partial_mat33f_sign() {
  float angle = static_cast<float>(kPi / 2.0);
  Vec3F axis(1.0f, 0.0f, 0.0f);
  QuaternionF q(axis, angle);

  Mat33F mat;
  q.ToPartialMatrix33F(mat);
  Vec3F v(0.0f, 1.0f, 0.0f);
  Vec3F result = mat * v;

  TEST_CHECK_(fabsf(result.x) < 0.001f,
      "Partial mat X-rot: result.x should be ~0, got %f", result.x);
  TEST_CHECK_(fabsf(result.y) < 0.001f,
      "Partial mat X-rot: result.y should be ~0, got %f", result.y);
  TEST_CHECK_(fabsf(result.z - 1.0f) < 0.001f,
      "Partial mat X-rot: result.z should be ~1, got %f", result.z);
}

// Bug: slerp computes normalizedA and normalizedB but then uses the
// original a and b in all subsequent calculations. The normalization is
// dead code.
//
// Test: slerp two non-unit quaternions that represent orthogonal rotations.
// At t=0, the result should be a unit quaternion equivalent to the first
// input rotation. The bug returns an unnormalized quaternion.
void test_quat_slerp_unnormalized() {
  // Identity rotation, scaled by 2. Normalized form: (0,0,0,1).
  QuaternionF a(0.0f, 0.0f, 0.0f, 2.0f);
  // 180-degree rotation around Y, unit length.
  QuaternionF b(0.0f, 1.0f, 0.0f, 0.0f);

  // dot(a,b) = 0*0 + 0*1 + 0*0 + 2*0 = 0, so this takes the slerp path
  // (not the linear fallback). alpha = acos(0) = pi/2.
  //
  // At t=0: result = a*cos(0) + c*sin(0) = a = (0,0,0,2).
  // A correct slerp should return a normalized quaternion ~(0,0,0,1).
  QuaternionF result = slerp(a, b, 0.0f);
  float modulus = result.Modulus();

  TEST_CHECK_(fabsf(modulus - 1.0f) < 0.01f,
      "slerp(t=0) should return a unit quaternion (modulus ~1), got %f. ",
      modulus);
}

// Bug: slerp does not handle negative dot products. When dot(a,b) < 0,
// one quaternion should be negated to interpolate along the shorter arc.
// Without this, slerp between antipodal quaternions (same rotation,
// opposite signs) produces NaN because it tries to normalize a zero vector.
//
// Test: (0,0,0,1) and (0,0,0,-1) represent the same rotation (identity).
// slerp at t=0.5 should produce identity. The bug produces NaN.
void test_quat_slerp_negative_dot() {
  QuaternionF a(0.0f, 0.0f, 0.0f, 1.0f);
  QuaternionF b(0.0f, 0.0f, 0.0f, -1.0f);

  QuaternionF result = slerp(a, b, 0.5f);

  // The result should be a valid quaternion, not NaN.
  bool is_finite = std::isfinite(result.x) && std::isfinite(result.y)
      && std::isfinite(result.z) && std::isfinite(result.w);
  TEST_CHECK_(is_finite,
      "slerp of antipodal quaternions should not produce NaN. "
      "Got (%f, %f, %f, %f).",
      result.x, result.y, result.z, result.w);

  if (is_finite) {
    // Both represent identity, so the interpolation should also be identity.
    float modulus = result.Modulus();
    TEST_CHECK_(fabsf(modulus - 1.0f) < 0.01f,
        "slerp of two identity quaternions should be unit length, got %f",
        modulus);
  }
}

// Bug: Inverse(Transform3F) was computed as Transform3F(-d, R^{-1}), simply
// negating the displacement.  The correct inverse of T(x) = R*x + d is
// T^{-1}(x) = R^{-1}*(x - d), which means the displacement must be
// -R^{-1}*d, not just -d.  The old code only worked when rotation was
// identity.
//
// Test: create a transform with a 90-degree rotation around Z and a non-zero
// displacement, apply it to a point, then apply the inverse.  The result
// should be the original point.
void test_transform3f_inverse() {
  // 90 degrees around Z: q = (0, 0, sin(pi/4), cos(pi/4))
  float s = sinf(3.14159265f / 4.0f);
  float c = cosf(3.14159265f / 4.0f);
  QuaternionF rot(0.0f, 0.0f, s, c);
  Vec3F disp(5.0f, 3.0f, -2.0f);
  Transform3F t(disp, rot);

  Vec3F original(1.0f, 2.0f, 3.0f);
  Vec3F transformed = t.Transform(original);
  Transform3F inv = Inverse(t);
  Vec3F recovered = inv.Transform(transformed);

  float dx = recovered.x - original.x;
  float dy = recovered.y - original.y;
  float dz = recovered.z - original.z;
  float error = sqrtf(dx * dx + dy * dy + dz * dz);

  TEST_CHECK_(error < 0.001f,
      "Inverse(t).Transform(t.Transform(p)) should return p. "
      "Original (%.3f, %.3f, %.3f), recovered (%.3f, %.3f, %.3f), error %.6f",
      original.x, original.y, original.z,
      recovered.x, recovered.y, recovered.z, error);
}

// Additional check: composing a transform with its inverse should produce
// identity (no rotation, zero displacement).
void test_transform3f_inverse_composition() {
  float s = sinf(3.14159265f / 3.0f);  // 60 degrees
  float c = cosf(3.14159265f / 3.0f);
  // Rotation around axis (1,1,0)/sqrt(2)
  float norm = 1.0f / sqrtf(2.0f);
  QuaternionF rot(s * norm, s * norm, 0.0f, c);
  Vec3F disp(10.0f, -7.0f, 4.0f);
  Transform3F t(disp, rot);

  Transform3F inv = Inverse(t);
  Transform3F composed = inv.Transform(t);

  // Displacement should be ~zero.
  float disp_len = sqrtf(composed.displacement.x * composed.displacement.x
      + composed.displacement.y * composed.displacement.y
      + composed.displacement.z * composed.displacement.z);
  TEST_CHECK_(disp_len < 0.001f,
      "Inverse(t).Transform(t) displacement should be ~zero, got (%.4f, %.4f, %.4f), "
      "length %.6f",
      composed.displacement.x, composed.displacement.y, composed.displacement.z,
      disp_len);

  // Rotation should be ~identity: (0, 0, 0, +/-1).
  float identity_error = sqrtf(composed.rotation.x * composed.rotation.x
      + composed.rotation.y * composed.rotation.y
      + composed.rotation.z * composed.rotation.z);
  TEST_CHECK_(identity_error < 0.001f,
      "Inverse(t).Transform(t) rotation should be ~identity, "
      "got (%.4f, %.4f, %.4f, %.4f)",
      composed.rotation.x, composed.rotation.y,
      composed.rotation.z, composed.rotation.w);
}

// Bug: piSkeleton::AddBone checks "if (parentID > 0)" instead of
// "if (parentID >= 0)".  This means bone 0 can never be a parent.
// When you add a bone with parentID=0 (intending it to be a child of the
// first bone), the condition 0 > 0 is false, so the new bone replaces
// mRoot.  The original root (bone 0) is orphaned and never traversed
// during Update().
//
// Test: create a 3-bone chain: bone 0 (root) -> bone 1 -> bone 2.
// Set bone 0 to translate by (10, 0, 0) and bone 1 to translate by (0, 5, 0).
// Bone 2's global matrix should include both parents' translations, giving
// (10, 5, 0).  With the bug, bone 0 is orphaned, so bone 2 only gets
// bone 1's translation (0, 5, 0).
void test_skeleton_bone0_cannot_be_parent() {
  piSkeleton skel;
  skel.Init(4);

  int b0 = skel.AddBone(-1);  // root
  int b1 = skel.AddBone(0);   // should be child of bone 0
  int b2 = skel.AddBone(1);   // child of bone 1

  // Bone 0: translate by (10, 0, 0)
  skel.UpdateBone(b0, SetTranslation(10.0f, 0.0f, 0.0f));
  // Bone 1: translate by (0, 5, 0)
  skel.UpdateBone(b1, SetTranslation(0.0f, 5.0f, 0.0f));
  // Bone 2: identity
  skel.UpdateBone(b2, SetIdentity());

  skel.Update();

  // Read back global matrices.
  Mat44F globals[3];
  skel.GetData(globals);

  // Bone 2's global matrix should be the composition of all parents.
  // Correct: translation(10, 5, 0).  Buggy: translation(0, 5, 0).
  Vec3F bone2_translation = ExtractTranslation(globals[2]);

  TEST_CHECK_(fabsf(bone2_translation.x - 10.0f) < 0.001f,
      "Bone 2 global translation X should be 10.0 (from bone 0), got %.3f. "
      "If 0.0, bone 0 was orphaned because parentID=0 failed the > 0 check.",
      bone2_translation.x);
  TEST_CHECK_(fabsf(bone2_translation.y - 5.0f) < 0.001f,
      "Bone 2 global translation Y should be 5.0 (from bone 1), got %.3f.",
      bone2_translation.y);
  TEST_CHECK_(fabsf(bone2_translation.z) < 0.001f,
      "Bone 2 global translation Z should be 0.0, got %.3f.",
      bone2_translation.z);
}

// Verify that bone 0's own global matrix is computed (i.e. bone 0 is
// reachable from mRoot).  With the bug, adding any bone with parentID=0
// replaces mRoot, so bone 0 is never visited by Update() and its
// global matrix remains uninitialized / zero.
void test_skeleton_root_not_orphaned() {
  piSkeleton skel;
  skel.Init(4);

  skel.AddBone(-1);  // bone 0, root
  skel.AddBone(0);   // bone 1, should be child of bone 0

  // Set bone 0 to a known translation.
  skel.UpdateBone(0, SetTranslation(7.0f, 3.0f, 1.0f));
  skel.UpdateBone(1, SetIdentity());

  skel.Update();

  Mat44F globals[2];
  skel.GetData(globals);

  // Bone 0 is root, so its global matrix == its local matrix.
  Vec3F bone0_translation = ExtractTranslation(globals[0]);

  TEST_CHECK_(fabsf(bone0_translation.x - 7.0f) < 0.001f,
      "Bone 0 global translation X should be 7.0, got %.3f. "
      "If wrong, bone 0 was not visited during Update() -- it was orphaned.",
      bone0_translation.x);
  TEST_CHECK_(fabsf(bone0_translation.y - 3.0f) < 0.001f,
      "Bone 0 global translation Y should be 3.0, got %.3f.",
      bone0_translation.y);
  TEST_CHECK_(fabsf(bone0_translation.z - 1.0f) < 0.001f,
      "Bone 0 global translation Z should be 1.0, got %.3f.",
      bone0_translation.z);
}

// ---------------------------------------------------------------------------
// Mat44F rotation consistency tests
// ---------------------------------------------------------------------------

// SetRotationX/Y should produce the same rotation as SetRotationAxisAngle4
// with the corresponding unit axis. SetRotationZ is already consistent.
//
// The bug: SetRotationX(t) and SetRotationY(t) rotate in the opposite
// direction compared to SetRotationAxisAngle4(axis, t). For example,
// SetRotationAxisAngle4(Vec3F(1,0,0), pi/4) applied to (0,1,0) gives
// (0, cos, sin) ~ (0, 0.707, 0.707), but SetRotationX(pi/4) applied to
// (0,1,0) gives (0, cos, -sin) ~ (0, 0.707, -0.707). The sign of sin is
// flipped in m[6] and m[9] for X, and similarly for Y.
void test_rotation_x_vs_axis_angle() {
  const float angle = 3.14159265f / 4.0f;  // 45 degrees
  Vec3F point(0.0f, 1.0f, 0.0f);

  Mat44F mat_x = SetRotationX(angle);
  Mat44F mat_aa = SetRotationAxisAngle4(Vec3F(1.0f, 0.0f, 0.0f), angle);

  Vec3F result_x = Transform(mat_x, point);
  Vec3F result_aa = Transform(mat_aa, point);

  float dx = result_x.x - result_aa.x;
  float dy = result_x.y - result_aa.y;
  float dz = result_x.z - result_aa.z;
  float error = sqrtf(dx * dx + dy * dy + dz * dz);

  TEST_CHECK_(error < 0.001f,
      "SetRotationX(pi/4) and SetRotationAxisAngle4(X, pi/4) should produce "
      "the same result for point (0,1,0). "
      "SetRotationX -> (%.4f, %.4f, %.4f), "
      "AxisAngle4   -> (%.4f, %.4f, %.4f), "
      "error = %.6f",
      result_x.x, result_x.y, result_x.z,
      result_aa.x, result_aa.y, result_aa.z,
      error);
}

void test_rotation_y_vs_axis_angle() {
  const float angle = 3.14159265f / 4.0f;  // 45 degrees
  Vec3F point(0.0f, 0.0f, 1.0f);

  Mat44F mat_y = SetRotationY(angle);
  Mat44F mat_aa = SetRotationAxisAngle4(Vec3F(0.0f, 1.0f, 0.0f), angle);

  Vec3F result_y = Transform(mat_y, point);
  Vec3F result_aa = Transform(mat_aa, point);

  float dx = result_y.x - result_aa.x;
  float dy = result_y.y - result_aa.y;
  float dz = result_y.z - result_aa.z;
  float error = sqrtf(dx * dx + dy * dy + dz * dz);

  TEST_CHECK_(error < 0.001f,
      "SetRotationY(pi/4) and SetRotationAxisAngle4(Y, pi/4) should produce "
      "the same result for point (0,0,1). "
      "SetRotationY -> (%.4f, %.4f, %.4f), "
      "AxisAngle4   -> (%.4f, %.4f, %.4f), "
      "error = %.6f",
      result_y.x, result_y.y, result_y.z,
      result_aa.x, result_aa.y, result_aa.z,
      error);
}

// SetRotationZ is already consistent with SetRotationAxisAngle4.
// This test serves as a control to confirm the test methodology is correct.
void test_rotation_z_vs_axis_angle() {
  const float angle = 3.14159265f / 4.0f;  // 45 degrees
  Vec3F point(1.0f, 0.0f, 0.0f);

  Mat44F mat_z = SetRotationZ(angle);
  Mat44F mat_aa = SetRotationAxisAngle4(Vec3F(0.0f, 0.0f, 1.0f), angle);

  Vec3F result_z = Transform(mat_z, point);
  Vec3F result_aa = Transform(mat_aa, point);

  float dx = result_z.x - result_aa.x;
  float dy = result_z.y - result_aa.y;
  float dz = result_z.z - result_aa.z;
  float error = sqrtf(dx * dx + dy * dy + dz * dz);

  TEST_CHECK_(error < 0.001f,
      "SetRotationZ(pi/4) and SetRotationAxisAngle4(Z, pi/4) should produce "
      "the same result for point (1,0,0). "
      "SetRotationZ -> (%.4f, %.4f, %.4f), "
      "AxisAngle4   -> (%.4f, %.4f, %.4f), "
      "error = %.6f",
      result_z.x, result_z.y, result_z.z,
      result_aa.x, result_aa.y, result_aa.z,
      error);
}

// Composing SetRotationX and SetRotationY should be equivalent to composing
// the corresponding SetRotationAxisAngle4 calls in the same order.
// This test catches the inconsistency in a more realistic usage scenario.
void test_rotation_xy_composition_vs_axis_angle() {
  const float angle_x = 3.14159265f / 6.0f;  // 30 degrees
  const float angle_y = 3.14159265f / 3.0f;  // 60 degrees
  Vec3F point(1.0f, 2.0f, 3.0f);

  Mat44F composed_xyz = SetRotationX(angle_x) * SetRotationY(angle_y);
  Mat44F composed_aa = SetRotationAxisAngle4(Vec3F(1.0f, 0.0f, 0.0f), angle_x)
                     * SetRotationAxisAngle4(Vec3F(0.0f, 1.0f, 0.0f), angle_y);

  Vec3F result_xyz = Transform(composed_xyz, point);
  Vec3F result_aa = Transform(composed_aa, point);

  float dx = result_xyz.x - result_aa.x;
  float dy = result_xyz.y - result_aa.y;
  float dz = result_xyz.z - result_aa.z;
  float error = sqrtf(dx * dx + dy * dy + dz * dz);

  TEST_CHECK_(error < 0.001f,
      "Composing SetRotationX * SetRotationY should match composing "
      "AxisAngle4(X) * AxisAngle4(Y). "
      "XY -> (%.4f, %.4f, %.4f), AA -> (%.4f, %.4f, %.4f), error = %.6f",
      result_xyz.x, result_xyz.y, result_xyz.z,
      result_aa.x, result_aa.y, result_aa.z,
      error);
}

void test_quat_to_axis_angle_acos_out_of_range() {
  // QuaternionF::ToAxisAngle calls acos(w) without clamping w to [-1, 1].
  // For unnormalized quaternions, or normalized ones that have drifted
  // past 1.0 due to floating-point accumulation, |w| > 1 causes acos()
  // to return NaN.

  // Construct a quaternion with w slightly above 1.0, simulating
  // floating-point drift after many rotations.
  QuaternionF q(0.0f, 0.0f, 0.0001f, 1.00001f);

  Vec3F axis;
  float angle;
  q.ToAxisAngle(axis, angle);

  // acos(1.00001) is undefined and returns NaN on IEEE 754 systems.
  // A robust implementation should clamp w to [-1, 1] and return a
  // valid angle.
  bool angle_is_nan = (angle != angle);  // NaN != NaN is true

  TEST_CHECK_(!angle_is_nan,
      "ToAxisAngle returned NaN angle for w=1.00001 (slightly above 1.0). "
      "acos(w) is undefined for |w| > 1; w should be clamped to [-1, 1].");

  // Also test w slightly below -1.0
  QuaternionF q2(0.0f, 0.0f, 0.0001f, -1.00001f);
  q2.ToAxisAngle(axis, angle);

  angle_is_nan = (angle != angle);
  TEST_CHECK_(!angle_is_nan,
      "ToAxisAngle returned NaN angle for w=-1.00001 (slightly below -1.0). "
      "acos(w) is undefined for |w| > 1; w should be clamped to [-1, 1].");
}

void test_quat_to_axis_angle_normalized_roundtrip() {
  // A properly normalized quaternion should round-trip through
  // ToAxisAngle without issues. This serves as a control test.
  Vec3F original_axis = Normalize(Vec3F(1.0f, 2.0f, 3.0f));
  float original_angle = 1.23f;
  QuaternionF q(original_axis, original_angle);

  Vec3F axis;
  float angle;
  q.ToAxisAngle(axis, angle);

  float angle_error = fabsf(angle - original_angle);
  float axis_error = sqrtf(
      (axis.x - original_axis.x) * (axis.x - original_axis.x) +
      (axis.y - original_axis.y) * (axis.y - original_axis.y) +
      (axis.z - original_axis.z) * (axis.z - original_axis.z));

  TEST_CHECK_(angle_error < 0.001f,
      "ToAxisAngle angle round-trip error = %.6f, expected < 0.001",
      angle_error);
  TEST_CHECK_(axis_error < 0.001f,
      "ToAxisAngle axis round-trip error = %.6f, expected < 0.001",
      axis_error);
}

void test_transform3f_scale_affects_point() {
  // Transform3F has a public `scale` member (default 1.0), but
  // Transform(Vec3F) ignores it entirely. Setting scale to 2.0
  // should double the point before applying rotation and displacement.

  Transform3F t;
  t.displacement = Vec3F(0.f, 0.f, 0.f);
  t.rotation.Clear();  // identity rotation
  t.scale = 2.0f;

  Vec3F input(1.f, 0.f, 0.f);
  Vec3F result = t.Transform(input);

  // With identity rotation, zero displacement, and scale=2:
  //   expected = rotate(input * scale) + displacement
  //            = (2, 0, 0) + (0, 0, 0) = (2, 0, 0)
  // But the bug makes Transform ignore scale, producing (1, 0, 0).

  TEST_CHECK_(fabsf(result.x - 2.0f) < 0.001f,
      "Transform(Vec3F) with scale=2 should produce x=2.0, got x=%.4f "
      "(scale member is ignored)",
      result.x);
}

void test_transform3f_scale_affects_transform_composition() {
  // When composing two transforms via Transform(Transform3F),
  // the parent's scale should affect the child's displacement.

  Transform3F parent;
  parent.displacement = Vec3F(0.f, 0.f, 0.f);
  parent.rotation.Clear();  // identity
  parent.scale = 3.0f;

  Transform3F child;
  child.displacement = Vec3F(1.f, 0.f, 0.f);
  child.rotation.Clear();  // identity
  child.scale = 1.0f;

  Transform3F composed = parent.Transform(child);

  // With identity parent rotation and parent scale=3:
  //   composed.displacement = parent.rotation.Rotate(child.displacement * parent.scale) + parent.displacement
  //                         = (3, 0, 0) + (0, 0, 0) = (3, 0, 0)
  //   composed.scale = parent.scale * child.scale = 3.0
  // But the bug makes Transform ignore scale, giving displacement=(1,0,0)
  // and not propagating scale at all.

  TEST_CHECK_(fabsf(composed.displacement.x - 3.0f) < 0.001f,
      "Parent scale=3 should scale child displacement to x=3.0, got x=%.4f "
      "(scale member is ignored in Transform(Transform3F))",
      composed.displacement.x);

  TEST_CHECK_(fabsf(composed.scale - 3.0f) < 0.001f,
      "Composed scale should be parent*child = 3.0, got %.4f "
      "(scale is not propagated in Transform(Transform3F))",
      composed.scale);
}

void test_transform3f_inverse_respects_scale() {
  // Inverse() also ignores scale. Applying a transform and then its
  // inverse should return to the original point, but this fails when
  // scale != 1.0.

  Transform3F t;
  t.displacement = Vec3F(1.f, 2.f, 3.f);
  t.rotation = QuaternionF(Normalize(Vec3F(0.f, 1.f, 0.f)), 0.5f);
  t.scale = 2.0f;

  Transform3F inv = Inverse(t);

  Vec3F original(4.f, 5.f, 6.f);
  Vec3F transformed = t.Transform(original);
  Vec3F roundtrip = inv.Transform(transformed);

  float dx = roundtrip.x - original.x;
  float dy = roundtrip.y - original.y;
  float dz = roundtrip.z - original.z;
  float error = sqrtf(dx*dx + dy*dy + dz*dz);

  TEST_CHECK_(error < 0.01f,
      "Transform then Inverse should round-trip with scale=2.0, "
      "but error=%.4f (Inverse ignores scale)",
      error);
}

// ============================================================
// Matrix / rotation consistency tests
// ============================================================

static float mat44_max_diff(const Mat44F &a, const Mat44F &b) {
  float d = 0.f;
  for (int i = 0; i < 16; ++i) {
    float ad = fabsf(a.m[i] - b.m[i]);
    if (ad > d) {
      d = ad;
    }
  }
  return d;
}

static Mat44F mat44_mul(const Mat44F &a, const Mat44F &b) {
  return a * b;
}

// 1. SetRotationQuaternion agrees with SetRotationAxisAngle4
void test_quat_matrix_vs_axis_angle() {
  Vec3F axis = Normalize(Vec3F(1.f, 2.f, 3.f));
  float angle = 0.73f;
  Mat44F from_aa = SetRotationAxisAngle4(axis, angle);
  Vec4F q(sinf(angle / 2) * axis.x,
          sinf(angle / 2) * axis.y,
          sinf(angle / 2) * axis.z,
          cosf(angle / 2));
  Mat44F from_q = SetRotationQuaternion(q);
  float d = mat44_max_diff(from_aa, from_q);
  TEST_CHECK_(d < 1e-5f,
      "SetRotationQuaternion vs SetRotationAxisAngle4: max diff=%.7f "
      "(should be consistent)", d);
}

// 2. SetRotationX/Y/Z agree with SetRotationAxisAngle4
void test_rotation_xyz_vs_axis_angle_consistency() {
  float t = 1.1f;
  float dx = mat44_max_diff(SetRotationX(t),
      SetRotationAxisAngle4(Vec3F(1, 0, 0), t));
  float dy = mat44_max_diff(SetRotationY(t),
      SetRotationAxisAngle4(Vec3F(0, 1, 0), t));
  float dz = mat44_max_diff(SetRotationZ(t),
      SetRotationAxisAngle4(Vec3F(0, 0, 1), t));
  TEST_CHECK_(dx < 1e-5f && dy < 1e-5f && dz < 1e-5f,
      "Rx/Ry/Rz vs AxisAngle: dx=%.7f dy=%.7f dz=%.7f", dx, dy, dz);
}

// 3. SetRotationEuler4 must equal Rz * Ry * Rx (same convention as all
//    other rotation-building functions in the engine).
void test_euler4_equals_composition() {
  float x = 0.3f, y = 0.5f, z = 0.7f;
  Mat44F euler = SetRotationEuler4(Vec3F(x, y, z));
  Mat44F composed = mat44_mul(SetRotationZ(z),
                   mat44_mul(SetRotationY(y), SetRotationX(x)));
  float d = mat44_max_diff(euler, composed);
  TEST_CHECK_(d < 1e-5f,
      "SetRotationEuler4(%.1f,%.1f,%.1f) must equal Rz*Ry*Rx, "
      "max diff=%.7f", x, y, z, d);
}

// 4. QuaternionF::ToMat33F matches top-left 3x3 of SetRotationQuaternion
void test_quat_tomat33_vs_setrotationquat() {
  Vec3F axis = Normalize(Vec3F(-1.f, 0.5f, 2.f));
  float angle = 1.2f;
  QuaternionF q(axis, angle);
  Mat33F m3 = q.ToMat33F();
  Vec4F q4(q.x, q.y, q.z, q.w);
  Mat44F m4 = SetRotationQuaternion(q4);
  float d = 0.f;
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      float ad = fabsf(m3.m[r * 3 + c] - m4.m[r * 4 + c]);
      if (ad > d) {
        d = ad;
      }
    }
  }
  TEST_CHECK_(d < 1e-5f,
      "ToMat33F vs SetRotationQuaternion(4x4): max diff=%.7f", d);
}

// 5. ExtractRotationEuler must round-trip with SetRotationEuler4:
//    ExtractRotationEuler(SetRotationEuler4(x,y,z)) == (x,y,z).
void test_extract_euler_roundtrip_euler4() {
  float x = 0.4f, y = 0.3f, z = 0.6f;
  Mat44F m = SetRotationEuler4(Vec3F(x, y, z));
  Vec3F e = ExtractRotationEuler(m);
  TEST_CHECK_(fabsf(e.x - x) < 1e-4f &&
              fabsf(e.y - y) < 1e-4f &&
              fabsf(e.z - z) < 1e-4f,
      "ExtractRotationEuler(SetRotationEuler4(%.1f,%.1f,%.1f)) = "
      "(%.4f,%.4f,%.4f) -- must recover original angles",
      x, y, z, e.x, e.y, e.z);
}

// 6. ExtractRotationEuler for a pure X-axis rotation must return (angle,0,0).
void test_extract_euler_pure_x() {
  float angle = 0.6f;
  Mat44F rx = SetRotationX(angle);
  Vec3F e = ExtractRotationEuler(rx);
  TEST_CHECK_(fabsf(e.x - angle) < 1e-4f &&
              fabsf(e.y) < 1e-4f &&
              fabsf(e.z) < 1e-4f,
      "ExtractRotationEuler(SetRotationX(%.1f)) = (%.4f,%.4f,%.4f) -- "
      "must be (%.1f, 0, 0)",
      angle, e.x, e.y, e.z, angle);
}

// 6b. ExtractRotationEuler for a pure Y-axis rotation must return (0,angle,0).
void test_extract_euler_pure_y() {
  float angle = 0.6f;
  Mat44F ry = SetRotationY(angle);
  Vec3F e = ExtractRotationEuler(ry);
  TEST_CHECK_(fabsf(e.x) < 1e-4f &&
              fabsf(e.y - angle) < 1e-4f &&
              fabsf(e.z) < 1e-4f,
      "ExtractRotationEuler(SetRotationY(%.1f)) = (%.4f,%.4f,%.4f) -- "
      "must be (0, %.1f, 0)",
      angle, e.x, e.y, e.z, angle);
}

// 6c. ExtractRotationEuler for a pure Z-axis rotation must return (0,0,angle).
void test_extract_euler_pure_z() {
  float angle = 0.6f;
  Mat44F rz = SetRotationZ(angle);
  Vec3F e = ExtractRotationEuler(rz);
  TEST_CHECK_(fabsf(e.x) < 1e-4f &&
              fabsf(e.y) < 1e-4f &&
              fabsf(e.z - angle) < 1e-4f,
      "ExtractRotationEuler(SetRotationZ(%.1f)) = (%.4f,%.4f,%.4f) -- "
      "must be (0, 0, %.1f)",
      angle, e.x, e.y, e.z, angle);
}

// 7. At gimbal lock (y ~ pi/2), ExtractRotationEuler must still recover y
//    and the sum x+z correctly (individual x,z are degenerate at gimbal lock
//    but y must be close to pi/2).
void test_extract_euler_gimbal_lock() {
  float x = 0.5f, z = 0.3f;
  float y = static_cast<float>(kPi) / 2.f;
  Mat44F m = SetRotationEuler4(Vec3F(x, y, z));
  Vec3F e = ExtractRotationEuler(m);
  TEST_CHECK_(fabsf(e.y - y) < 0.05f,
      "ExtractRotationEuler at gimbal lock: extracted y=%.4f, "
      "expected ~%.4f",
      e.y, y);
}

// 7b. ExtractRotationEuler must not depend on translation components.
//     Adding translation to a rotation matrix must not change the result.
//     Test several rotations: Rx hits the m[0]==1 branch, Ry/Rz and a
//     combined rotation hit the general branch.
void test_extract_euler_ignores_translation() {
  Mat44F rotations[] = {
    SetRotationX(0.5f),
    SetRotationY(0.7f),
    SetRotationZ(0.3f),
    SetRotationAxisAngle4(Normalize(Vec3F(1, 2, 3)), 0.9f),
  };
  const char *names[] = {"Rx", "Ry", "Rz", "AxisAngle"};
  for (int i = 0; i < 4; ++i) {
    Mat44F m = rotations[i];
    Vec3F e1 = ExtractRotationEuler(m);
    m.m[3] = 10.0f;
    m.m[7] = 20.0f;
    m.m[11] = 30.0f;
    Vec3F e2 = ExtractRotationEuler(m);
    TEST_CHECK_(fabsf(e1.x - e2.x) < 1e-6f &&
                fabsf(e1.y - e2.y) < 1e-6f &&
                fabsf(e1.z - e2.z) < 1e-6f,
        "%s: ExtractRotationEuler must not depend on translation: "
        "without=(%.4f,%.4f,%.4f) with=(%.4f,%.4f,%.4f)",
        names[i], e1.x, e1.y, e1.z, e2.x, e2.y, e2.z);
  }
}

// 7c. The gimbal-lock branch uses m[11] (translation element [2][3]) instead
//     of a rotation element. For orthogonal matrices this is masked because
//     m[0]==1 forces m[2]==0, making atan2(0, m[11])=0. But for a scaled
//     rotation (realistic: model matrix = Scale * Rotation), m[0] can be 1
//     while m[2]!=0, and then m[11] leaks into the result.
//     Example: Ry(a) * Scale(1/cos(a), 1, 1) gives m[0]=1, m[2]=sin(a)!=0.
void test_extract_euler_gimbal_branch_reads_m11() {
  float a = 0.4f;
  Mat44F m = SetRotationY(a) * SetScale4(1.f / cosf(a), 1.f, 1.f);
  Vec3F e1 = ExtractRotationEuler(m);
  m.m[11] = 50.0f;
  Vec3F e2 = ExtractRotationEuler(m);
  TEST_CHECK_(fabsf(e1.x - e2.x) < 1e-6f &&
              fabsf(e1.y - e2.y) < 1e-6f &&
              fabsf(e1.z - e2.z) < 1e-6f,
      "Scaled Ry: translation m[11] must not affect extraction: "
      "without=(%.4f,%.4f,%.4f) with=(%.4f,%.4f,%.4f)",
      e1.x, e1.y, e1.z, e2.x, e2.y, e2.z);
}

// 8. M * Transpose(M) should be identity for all rotation functions
//    (orthogonality check).
void test_rotation_matrices_are_orthogonal() {
  float t = 0.8f;
  Vec3F axis = Normalize(Vec3F(1, 1, 1));

  auto check_ortho = [](const Mat44F &m, const char *name) {
    Mat44F mt = Transpose(m);
    Mat44F prod = m * mt;
    Mat44F id = SetIdentity();
    float d = mat44_max_diff(prod, id);
    TEST_CHECK_(d < 1e-5f,
        "%s: M * M^T should be identity, max diff=%.7f", name, d);
  };

  check_ortho(SetRotationX(t), "SetRotationX");
  check_ortho(SetRotationY(t), "SetRotationY");
  check_ortho(SetRotationZ(t), "SetRotationZ");
  check_ortho(SetRotationAxisAngle4(axis, t), "SetRotationAxisAngle4");
  check_ortho(SetRotationEuler4(Vec3F(0.3f, 0.5f, 0.7f)), "SetRotationEuler4");

  Vec4F q(sinf(t / 2) * axis.x, sinf(t / 2) * axis.y,
          sinf(t / 2) * axis.z, cosf(t / 2));
  check_ortho(SetRotationQuaternion(q), "SetRotationQuaternion");
}

// 9. Translation matrix: Transform(SetTranslation(t), v) == v + t
void test_translation_transforms_point() {
  Vec3F t(3.f, -1.f, 7.f);
  Vec3F v(1.f, 2.f, 3.f);
  Mat44F m = SetTranslation(t);
  Vec3F result = Transform(m, v);
  float d = Length(result - (v + t));
  TEST_CHECK_(d < 1e-5f,
      "SetTranslation * point: error=%.7f", d);
}

// 10. SetLookat produces orthonormal basis (post-fix for bug 23)
void test_lookat_orthonormal() {
  Vec3F eye(1, 2, 3);
  Vec3F target(4, 5, 6);
  Vec3F up(0, 1, 0);
  Mat44F m = SetLookat(eye, target, up);

  Vec3F row0(m.m[0], m.m[1], m.m[2]);
  Vec3F row1(m.m[4], m.m[5], m.m[6]);
  Vec3F row2(m.m[8], m.m[9], m.m[10]);

  float len0 = Length(row0);
  float len1 = Length(row1);
  float len2 = Length(row2);
  float dot01 = Dot(row0, row1);
  float dot02 = Dot(row0, row2);
  float dot12 = Dot(row1, row2);

  TEST_CHECK_(fabsf(len0 - 1.f) < 1e-5f &&
              fabsf(len1 - 1.f) < 1e-5f &&
              fabsf(len2 - 1.f) < 1e-5f,
      "SetLookat rows unit length: %.6f %.6f %.6f", len0, len1, len2);
  TEST_CHECK_(fabsf(dot01) < 1e-5f &&
              fabsf(dot02) < 1e-5f &&
              fabsf(dot12) < 1e-5f,
      "SetLookat rows orthogonal: dots=%.6f %.6f %.6f",
      dot01, dot02, dot12);
}

// 11. SetLookat with eye==target returns identity (bug 23 fix)
void test_lookat_degenerate_returns_identity() {
  Vec3F p(5, 5, 5);
  Mat44F m = SetLookat(p, p, Vec3F(0, 1, 0));
  Mat44F id = SetIdentity();
  float d = mat44_max_diff(m, id);
  TEST_CHECK_(d < 1e-5f,
      "SetLookat(eye==target) should be identity, diff=%.7f", d);
}

// Bug 46: SetPerspective uses tan(fovy) instead of tan(fovy/2).
// The vertical scale element m[5] must equal 1/tan(fovy/2).
// With the bug, it equals 1/tan(fovy) -- a completely different value.
void test_perspective_y_equals_cot_half_fovy() {
  const float fovy = 60.0f;
  const float aspect = 1.5f;
  const float znear = 0.1f;
  const float zfar = 100.0f;

  Mat44F m = SetPerspective(fovy, aspect, znear, zfar);

  float half_fovy_rad = fovy * static_cast<float>(M_PI) / 360.0f;
  float expected_y = 1.0f / tanf(half_fovy_rad);
  float expected_x = expected_y / aspect;

  TEST_CHECK_(fabsf(m.m[5] - expected_y) < 1e-5f,
      "m[5] should be cot(fovy/2)=%.6f, got %.6f", expected_y, m.m[5]);
  TEST_CHECK_(fabsf(m.m[0] - expected_x) < 1e-5f,
      "m[0] should be cot(fovy/2)/aspect=%.6f, got %.6f", expected_x, m.m[0]);
}

// SetPerspective and SetFrustumPerspective accept the same fovy parameter
// and must produce the same projection matrix for the same inputs.
void test_perspective_matches_frustum_perspective() {
  const float fovy = 90.0f;
  const float aspect = 16.0f / 9.0f;
  const float znear = 0.5f;
  const float zfar = 500.0f;

  Mat44F mat = SetPerspective(fovy, aspect, znear, zfar);
  Frustum3F fru = SetFrustumPerspective(fovy, aspect, znear, zfar);
  Mat44F fru_mat = fru.matrix;

  float max_diff = 0.0f;
  for (int i = 0; i < 16; ++i) {
    float d = fabsf(mat.m[i] - fru_mat.m[i]);
    if (d > max_diff) {
      max_diff = d;
    }
  }
  TEST_CHECK_(max_diff < 1e-5f,
      "SetPerspective and SetFrustumPerspective must match, max diff=%.7f",
      max_diff);
}

// SetPerspectiveTiled with offset=(0,0) and size=(1,1) is a full tile,
// which must produce the same matrix as SetPerspective.
void test_perspective_tiled_matches_perspective() {
  const float fovy = 75.0f;
  const float aspect = 2.0f;
  const float znear = 1.0f;
  const float zfar = 1000.0f;

  Mat44F mat = SetPerspective(fovy, aspect, znear, zfar);
  Mat44F tiled = SetPerspectiveTiled(fovy, aspect, znear, zfar,
      Vec2F(0.0f, 0.0f), Vec2F(1.0f, 1.0f));

  float max_diff = 0.0f;
  for (int i = 0; i < 16; ++i) {
    float d = fabsf(mat.m[i] - tiled.m[i]);
    if (d > max_diff) {
      max_diff = d;
    }
  }
  TEST_CHECK_(max_diff < 1e-5f,
      "SetPerspectiveTiled(full) must match SetPerspective, max diff=%.7f",
      max_diff);
}

void test_ortho_symmetric_corners(void) {
  float L = -5.f, R = 5.f, B = -3.f, T = 3.f, N = 1.f, F = 100.f;
  Mat44F m = SetOrtho(L, R, B, T, N, F);

  Vec3F lb_near = Transform(m, Vec3F(L, B, -N));
  Vec3F rt_near = Transform(m, Vec3F(R, T, -N));
  Vec3F lb_far  = Transform(m, Vec3F(L, B, -F));
  Vec3F rt_far  = Transform(m, Vec3F(R, T, -F));

  float eps = 1e-5f;
  TEST_CHECK_(fabsf(lb_near.x - (-1.f)) < eps, "left-bottom-near x: got %f, want -1", lb_near.x);
  TEST_CHECK_(fabsf(lb_near.y - (-1.f)) < eps, "left-bottom-near y: got %f, want -1", lb_near.y);
  TEST_CHECK_(fabsf(lb_near.z - (-1.f)) < eps, "left-bottom-near z: got %f, want -1", lb_near.z);

  TEST_CHECK_(fabsf(rt_near.x - 1.f) < eps, "right-top-near x: got %f, want 1", rt_near.x);
  TEST_CHECK_(fabsf(rt_near.y - 1.f) < eps, "right-top-near y: got %f, want 1", rt_near.y);
  TEST_CHECK_(fabsf(rt_near.z - (-1.f)) < eps, "right-top-near z: got %f, want -1", rt_near.z);

  TEST_CHECK_(fabsf(lb_far.z - 1.f) < eps, "left-bottom-far z: got %f, want 1", lb_far.z);
  TEST_CHECK_(fabsf(rt_far.z - 1.f) < eps, "right-top-far z: got %f, want 1", rt_far.z);
}

void test_ortho_asymmetric_corners(void) {
  float L = -2.f, R = 10.f, B = -3.f, T = 7.f, N = 1.f, F = 50.f;
  Mat44F m = SetOrtho(L, R, B, T, N, F);

  Vec3F bl = Transform(m, Vec3F(L, B, -N));
  Vec3F tr = Transform(m, Vec3F(R, T, -F));
  float cx = (L + R) * 0.5f;
  float cy = (B + T) * 0.5f;
  float cz = -(N + F) * 0.5f;
  Vec3F mid = Transform(m, Vec3F(cx, cy, cz));

  float eps = 1e-4f;
  TEST_CHECK_(fabsf(bl.x - (-1.f)) < eps,
    "bottom-left x: got %f, want -1", bl.x);
  TEST_CHECK_(fabsf(bl.y - (-1.f)) < eps,
    "bottom-left y: got %f, want -1", bl.y);
  TEST_CHECK_(fabsf(bl.z - (-1.f)) < eps,
    "bottom-left z: got %f, want -1", bl.z);

  TEST_CHECK_(fabsf(tr.x - 1.f) < eps,
    "top-right x: got %f, want 1", tr.x);
  TEST_CHECK_(fabsf(tr.y - 1.f) < eps,
    "top-right y: got %f, want 1", tr.y);
  TEST_CHECK_(fabsf(tr.z - 1.f) < eps,
    "top-right z: got %f, want 1", tr.z);

  TEST_CHECK_(fabsf(mid.x) < eps,
    "center x: got %f, want 0", mid.x);
  TEST_CHECK_(fabsf(mid.y) < eps,
    "center y: got %f, want 0", mid.y);
  TEST_CHECK_(fabsf(mid.z) < eps,
    "center z: got %f, want 0", mid.z);
}

void test_ortho_center_maps_to_origin(void) {
  float L = 10.f, R = 50.f, B = -20.f, T = 80.f, N = 1.f, F = 500.f;
  Mat44F m = SetOrtho(L, R, B, T, N, F);

  float cx = (L + R) * 0.5f;
  float cy = (B + T) * 0.5f;
  float cz = -(N + F) * 0.5f;
  Vec3F center = Transform(m, Vec3F(cx, cy, cz));

  float eps = 1e-4f;
  TEST_CHECK_(fabsf(center.x) < eps,
    "center of box x: got %f, want 0", center.x);
  TEST_CHECK_(fabsf(center.y) < eps,
    "center of box y: got %f, want 0", center.y);
  TEST_CHECK_(fabsf(center.z) < eps,
    "center of box z: got %f, want 0", center.z);
}

void test_ortho_consistent_with_perspective(void) {
  float N = 1.f, F = 100.f;
  float aspect = 16.f / 9.f;
  float fovy = 60.f * kPi / 180.f;
  float half_h = N * tanf(fovy * 0.5f);
  float half_w = half_h * aspect;

  Mat44F ortho = SetOrtho(-half_w, half_w, -half_h, half_h, N, F);
  Mat44F persp = SetPerspective(fovy * 180.f / kPi, aspect, N, F);

  float eps = 1e-5f;
  TEST_CHECK_(fabsf(ortho.m[0] - persp.m[0] / N) < eps,
    "ortho x-scale should be persp x-scale / near: ortho=%f, persp/n=%f",
    ortho.m[0], persp.m[0] / N);
  TEST_CHECK_(fabsf(ortho.m[5] - persp.m[5] / N) < eps,
    "ortho y-scale should be persp y-scale / near: ortho=%f, persp/n=%f",
    ortho.m[5], persp.m[5] / N);

  TEST_CHECK_(fabsf(ortho.m[3]) < eps,
    "symmetric ortho x-translation should be 0: got %f", ortho.m[3]);
  TEST_CHECK_(fabsf(ortho.m[7]) < eps,
    "symmetric ortho y-translation should be 0: got %f", ortho.m[7]);
}
