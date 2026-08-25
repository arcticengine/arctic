#ifndef ENGINE_MODEL_H_
#define ENGINE_MODEL_H_

#include <memory>
#include <string>
#include <vector>

#include "engine/gl_texture2d.h"
#include "engine/gl_texture_cache.h"
#include "engine/mat44f.h"
#include "engine/mesh.h"
#include "engine/vec3f.h"

namespace arctic {

/// @addtogroup global_advanced
/// @{

MeshVertexFormat MeshFormatPosNormUv();

struct SkinInfluence {
  Si32 bone = -1;
  float weight = 0.0f;
};

struct Bone {
  std::string name;
  Si32 parent = -1;
  Mat44F local_bind;
  Mat44F inverse_bind;
};

struct AnimCurve {
  Si32 bone = -1;
  std::string property;
  Si32 axis = 0;
  std::vector<Si64> times;
  std::vector<float> values;
};

struct AnimClip {
  std::string name;
  std::vector<AnimCurve> curves;
};

struct AnimTake {
  std::string name;
  double local_from = 0.0;
  double local_to = 0.0;
};

struct ModelPart {
  std::unique_ptr<Mesh> mesh;
  std::shared_ptr<GlTexture2D> diffuse;
  std::shared_ptr<GlTexture2D> normal;
  Vec3F color = Vec3F(1.0f, 1.0f, 1.0f);
  bool has_alpha = false;
  std::string raw_tex;
  std::string resolved_tex;
  std::string raw_normal;
  std::string resolved_normal;
  std::string node_name;
  std::string material_name;
  // One list per vertex of mesh, in the same order. Empty on a rigid part.
  std::vector<std::vector<SkinInfluence> > skin;
};

struct Model {
  std::vector<ModelPart> parts;
  std::vector<Bone> bones;
  std::vector<AnimClip> clips;
  std::vector<AnimTake> takes;
  std::vector<std::string> texture_clips;
  bool z_up = false;
  float unit_scale = 1.0f;
  std::shared_ptr<GlTextureCache> cache;
};

/// @}

}  // namespace arctic

#endif  // ENGINE_MODEL_H_
