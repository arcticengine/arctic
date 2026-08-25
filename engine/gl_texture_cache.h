#ifndef ENGINE_GL_TEXTURE_CACHE_H_
#define ENGINE_GL_TEXTURE_CACHE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "engine/arctic_types.h"
#include "engine/gl_texture2d.h"

namespace arctic {

/// @addtogroup global_advanced
/// @{

/// A GPU texture plus whether it has a hole in the alpha. The cache is a
/// load-time table: Draw holds the pointer from here and never looks a name
/// up again.
struct GlTextureCacheEntry {
  std::shared_ptr<GlTexture2D> texture;
  bool has_alpha = false;
};

class GlTextureCache {
 public:
  // Decodes the file and, when a GL context exists, uploads it. A second
  // call with a path that canonicalizes to the same file returns the same
  // pointer. An empty or unreadable path returns a null texture.
  GlTextureCacheEntry Load(const char *path);

  // Looks the canonical path up without reading the disk.
  GlTextureCacheEntry Find(const char *path) const;

  // 2x2 opaque white. Stored on its own, not under the string "white".
  std::shared_ptr<GlTexture2D> White();

 private:
  std::string MakeKey(const char *path) const;
  void Upload(GlTexture2D *tex, const void *pixels, Si32 w, Si32 h);

  std::map<std::string, GlTextureCacheEntry> entries_;
  std::shared_ptr<GlTexture2D> white_;
};

// Turns a path written by a DCC (often with backslashes and the wrong
// folder) into a file that exists. search_dirs are tried with the basename,
// then stem+".png"/".tga", then a case-insensitive and stem-containment
// walk of each directory. An empty string means nothing was found.
std::string ResolveAssetPath(const std::string &raw,
    const std::vector<std::string> &search_dirs);

/// @}

}  // namespace arctic

#endif  // ENGINE_GL_TEXTURE_CACHE_H_
