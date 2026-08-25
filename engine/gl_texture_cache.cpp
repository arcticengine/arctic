#include "engine/gl_texture_cache.h"

#include <cctype>
#include <sstream>

#include "engine/arctic_platform.h"
#include "engine/easy_advanced.h"
#include "engine/easy_sprite.h"
#include "engine/engine.h"
#include "engine/log.h"
#include "engine/opengl.h"
#include "engine/rgba.h"

namespace arctic {

static bool CanUploadGpu() {
  Engine *engine = GetEngine();
  if (engine == nullptr) {
    return false;
  }
  if (engine->IsSoftwareOnly()) {
    return false;
  }
  return true;
}

static std::string LowerCopy(std::string s) {
  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (c >= 'A' && c <= 'Z') {
      s[i] = static_cast<char>(c - 'A' + 'a');
    }
  }
  return s;
}

static std::string ReplaceBackslash(std::string s) {
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '\\') {
      s[i] = '/';
    }
  }
  return s;
}

static std::string BasenameOf(const std::string &path) {
  size_t slash = path.find_last_of("/\\");
  if (slash == std::string::npos) {
    return path;
  }
  return path.substr(slash + 1);
}

static std::string StemOf(const std::string &name) {
  size_t dot = name.find_last_of('.');
  if (dot == std::string::npos) {
    return name;
  }
  return name.substr(0, dot);
}

static std::string JoinDir(const std::string &dir, const std::string &name) {
  if (dir.empty()) {
    return name;
  }
  char last = dir[dir.size() - 1];
  if (last == '/' || last == '\\') {
    return dir + name;
  }
  return dir + "/" + name;
}

static bool FileIsThere(const std::string &path) {
  if (path.empty()) {
    return false;
  }
  return DoesFileExist(path.c_str()) == kTrivalentTrue;
}

static bool HasAlpha(const Sprite &spr) {
  if (spr.Width() < 1 || spr.Height() < 1) {
    return false;
  }
  const Rgba *px = spr.RgbaData();
  Si32 n = spr.Width() * spr.Height();
  for (Si32 i = 0; i < n; ++i) {
    if (px[i].a < 250) {
      return true;
    }
  }
  return false;
}

static std::string AsciiKeep(const std::string &s) {
  std::string o;
  o.reserve(s.size());
  for (size_t i = 0; i < s.size(); ++i) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    if (c >= 'A' && c <= 'Z') {
      o += static_cast<char>(c - 'A' + 'a');
    } else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
      o += static_cast<char>(c);
    }
  }
  return o;
}

std::string GlTextureCache::MakeKey(const char *path) const {
  if (path == nullptr || path[0] == 0) {
    return std::string();
  }
  return CanonicalizePath(path);
}

void GlTextureCache::Upload(GlTexture2D *tex, const void *pixels,
    Si32 w, Si32 h) {
  tex->Create(w, h);
  tex->Bind(0);
  tex->SetData(pixels, w, h);
  tex->SetFilterMode(kFilterBilinear);
  ARCTIC_GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
      GL_REPEAT));
  ARCTIC_GL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
      GL_REPEAT));
}

GlTextureCacheEntry GlTextureCache::Load(const char *path) {
  GlTextureCacheEntry miss;
  if (path == nullptr || path[0] == 0) {
    return miss;
  }
  std::string key = MakeKey(path);
  if (!key.empty()) {
    std::map<std::string, GlTextureCacheEntry>::iterator it =
        entries_.find(key);
    if (it != entries_.end()) {
      return it->second;
    }
  }
  if (!FileIsThere(path)) {
    return miss;
  }
  Sprite spr;
  spr.Load(path);
  if (spr.Width() < 1 || spr.Height() < 1) {
    *Log() << "Texture failed: " << path;
    return miss;
  }
  GlTextureCacheEntry entry;
  entry.has_alpha = HasAlpha(spr);
  entry.texture.reset(new GlTexture2D());
  if (CanUploadGpu()) {
    Upload(entry.texture.get(), spr.RgbaData(), spr.Width(), spr.Height());
  }
  if (key.empty()) {
    key = MakeKey(path);
  }
  if (key.empty()) {
    key = path;
  }
  entries_[key] = entry;
  *Log() << "Texture " << path << " " << spr.Width() << "x" << spr.Height()
    << (entry.has_alpha ? " alpha" : "");
  return entry;
}

GlTextureCacheEntry GlTextureCache::Find(const char *path) const {
  GlTextureCacheEntry miss;
  std::string key = MakeKey(path);
  if (key.empty()) {
    return miss;
  }
  std::map<std::string, GlTextureCacheEntry>::const_iterator it =
      entries_.find(key);
  if (it == entries_.end()) {
    return miss;
  }
  return it->second;
}

std::shared_ptr<GlTexture2D> GlTextureCache::White() {
  if (white_) {
    return white_;
  }
  Ui8 pixels[16] = {
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255
  };
  white_.reset(new GlTexture2D());
  if (CanUploadGpu()) {
    Upload(white_.get(), pixels, 2, 2);
  }
  return white_;
}

std::string ResolveAssetPath(const std::string &raw,
    const std::vector<std::string> &search_dirs) {
  if (raw.empty()) {
    return std::string();
  }
  std::string norm = ReplaceBackslash(raw);
  if (FileIsThere(norm)) {
    return norm;
  }
  if (FileIsThere(raw)) {
    return raw;
  }
  std::string base = BasenameOf(norm);
  std::string stem = StemOf(base);
  std::string stem_low = LowerCopy(stem);
  std::string want = AsciiKeep(stem);
  std::vector<std::string> dirs = search_dirs;
  if (dirs.empty()) {
    dirs.push_back(".");
  }
  const char *exts[] = {".png", ".tga", ".jpg", ".jpeg", nullptr};
  for (size_t d = 0; d < dirs.size(); ++d) {
    std::vector<DirectoryEntry> entries;
    if (!GetDirectoryEntries(dirs[d].c_str(), &entries)) {
      continue;
    }
    std::string exact;
    std::string stem_ext;
    std::string case_hit;
    std::string keep_hit;
    std::string contain_hit;
    for (size_t i = 0; i < entries.size(); ++i) {
      if (entries[i].is_file == kTrivalentFalse) {
        continue;
      }
      const std::string &name = entries[i].title;
      if (name.empty() || name[0] == '.') {
        continue;
      }
      std::string path = JoinDir(dirs[d], name);
      if (name == base) {
        exact = path;
        break;
      }
      std::string nstem = StemOf(name);
      std::string nlow = LowerCopy(nstem);
      std::string nkeep = AsciiKeep(nstem);
      for (Si32 e = 0; exts[e] != nullptr; ++e) {
        if (name == stem + exts[e]) {
          stem_ext = path;
        }
      }
      if (case_hit.empty() && !stem_low.empty() && nlow == stem_low) {
        case_hit = path;
      }
      if (keep_hit.empty() && !want.empty() && nkeep == want) {
        keep_hit = path;
      }
      if (contain_hit.empty() && !want.empty() && !nkeep.empty()) {
        if (nkeep.find(want) != std::string::npos ||
            want.find(nkeep) != std::string::npos) {
          contain_hit = path;
        }
      }
    }
    if (!exact.empty()) {
      return exact;
    }
    if (!stem_ext.empty()) {
      return stem_ext;
    }
    if (!case_hit.empty()) {
      return case_hit;
    }
    if (!keep_hit.empty()) {
      return keep_hit;
    }
    if (!contain_hit.empty()) {
      return contain_hit;
    }
  }
  return std::string();
}

}  // namespace arctic
