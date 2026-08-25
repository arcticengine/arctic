#ifndef ENGINE_MESH_FBX_H_
#define ENGINE_MESH_FBX_H_

#include <string>
#include <vector>

#include "engine/arctic_types.h"
#include "engine/gl_texture_cache.h"
#include "engine/model.h"

namespace arctic {

/// @addtogroup global_advanced
/// @{

struct FbxLoadOptions {
  std::vector<std::string> search_dirs;
  bool apply_unit_scale = true;
  bool upload_textures = true;
  std::string (*remap)(const std::string &raw,
      const std::string &material_name, void *user) = nullptr;
  void *remap_user = nullptr;
  GlTextureCache *cache = nullptr;
};

bool LoadFbx(Model *out, const Ui8 *data, Si32 size,
    const FbxLoadOptions &opt);
bool LoadFbx(Model *out, const char *path, const FbxLoadOptions &opt);

/// @}

}  // namespace arctic

#endif  // ENGINE_MESH_FBX_H_
