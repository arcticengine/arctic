#include "engine/mesh_fbx.h"

#include <cmath>
#include <map>
#include <sstream>

#include "engine/easy_files.h"
#include "engine/log.h"
#include "engine/ofbx.h"
#include "engine/vec2f.h"

namespace arctic {

namespace {

ofbx::Matrix44D Mul44(const ofbx::Matrix44D &a, const ofbx::Matrix44D &b) {
  ofbx::Matrix44D r;
  for (int c = 0; c < 4; ++c) {
    for (int row = 0; row < 4; ++row) {
      double s = 0.0;
      for (int k = 0; k < 4; ++k) {
        s += a.m[k * 4 + row] * b.m[c * 4 + k];
      }
      r.m[c * 4 + row] = s;
    }
  }
  return r;
}

Vec3F XformPoint(const ofbx::Matrix44D &m, const Vec3D &p) {
  return Vec3F(
      static_cast<float>(m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12]),
      static_cast<float>(m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13]),
      static_cast<float>(m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14]));
}

Vec3F XformDir(const ofbx::Matrix44D &m, const Vec3D &p) {
  return Vec3F(
      static_cast<float>(m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z),
      static_cast<float>(m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z),
      static_cast<float>(m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z));
}

Vec3F MaybeZup(const Vec3F &p, bool z_up) {
  if (!z_up) {
    return p;
  }
  return Vec3F(p.x, p.z, -p.y);
}

Mat44F OfbxToMat44(const ofbx::Matrix44D &m, bool z_up) {
  // ofbx is column-major with translation in m[12..14]; Mat44F is row-major
  // with translation in m[3], m[7], m[11].
  Mat44F a(
      static_cast<float>(m.m[0]), static_cast<float>(m.m[4]),
      static_cast<float>(m.m[8]), static_cast<float>(m.m[12]),
      static_cast<float>(m.m[1]), static_cast<float>(m.m[5]),
      static_cast<float>(m.m[9]), static_cast<float>(m.m[13]),
      static_cast<float>(m.m[2]), static_cast<float>(m.m[6]),
      static_cast<float>(m.m[10]), static_cast<float>(m.m[14]),
      static_cast<float>(m.m[3]), static_cast<float>(m.m[7]),
      static_cast<float>(m.m[11]), static_cast<float>(m.m[15]));
  if (!z_up) {
    return a;
  }
  Mat44F s = SetSwapYZ();
  Mat44F inv(
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, -1.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f);
  return s * a * inv;
}

Mat44F Identity44() {
  return SetIdentity4();
}

std::string DataViewStr(const ofbx::DataView &dv) {
  char name[256];
  name[0] = 0;
  dv.toString(name);
  return name;
}

std::string TexturePathOf(const ofbx::Texture *td) {
  if (!td) {
    return "";
  }
  std::string n = DataViewStr(td->getRelativeFileName());
  if (n.empty()) {
    n = DataViewStr(td->getFileName());
  }
  if (n.empty()) {
    for (int i = 0; i < 8; ++i) {
      ofbx::Object *link = td->resolveObjectLink(i);
      if (!link) {
        break;
      }
      if (link->getType() == ofbx::Object::Type::VIDEO) {
        const ofbx::Video *v = static_cast<const ofbx::Video*>(link);
        n = DataViewStr(v->getRelativeFileName());
        if (n.empty()) {
          n = DataViewStr(v->getFileName());
        }
        if (!n.empty()) {
          break;
        }
      }
    }
  }
  if (n.empty() && td->name[0]) {
    n = td->name;
  }
  return n;
}

std::string ResolveMaterialTex(const ofbx::Material *mat,
    ofbx::Texture::TextureType slot) {
  if (!mat) {
    return "";
  }
  const ofbx::Texture *td = mat->getTexture(slot);
  std::string tex = TexturePathOf(td);
  if (!tex.empty()) {
    return tex;
  }
  if (slot != ofbx::Texture::DIFFUSE) {
    return "";
  }
  for (int i = 0; i < 32; ++i) {
    ofbx::Object *link = mat->resolveObjectLink(i);
    if (!link) {
      break;
    }
    if (link->getType() == ofbx::Object::Type::TEXTURE) {
      tex = TexturePathOf(static_cast<const ofbx::Texture*>(link));
    } else if (link->getType() == ofbx::Object::Type::LAYERED_TEXTURE) {
      for (int j = 0; j < 8; ++j) {
        ofbx::Object *inner = link->resolveObjectLink(j);
        if (!inner) {
          break;
        }
        if (inner->getType() == ofbx::Object::Type::TEXTURE) {
          tex = TexturePathOf(static_cast<const ofbx::Texture*>(inner));
          if (!tex.empty()) {
            break;
          }
        }
      }
    } else if (link->getType() == ofbx::Object::Type::VIDEO) {
      const ofbx::Video *v = static_cast<const ofbx::Video*>(link);
      tex = DataViewStr(v->getRelativeFileName());
      if (tex.empty()) {
        tex = DataViewStr(v->getFileName());
      }
    }
    if (!tex.empty()) {
      return tex;
    }
  }
  return "";
}

void CatalogClips(ofbx::IScene *scene, std::vector<std::string> *clips) {
  int n = scene->getAllObjectCount();
  const ofbx::Object *const *objs = scene->getAllObjects();
  for (int i = 0; i < n; ++i) {
    const ofbx::Object *o = objs[i];
    if (!o) {
      continue;
    }
    ofbx::Object::Type ty = o->getType();
    if (ty == ofbx::Object::Type::TEXTURE) {
      std::string p = TexturePathOf(static_cast<const ofbx::Texture*>(o));
      if (!p.empty()) {
        clips->push_back(p);
      }
    } else if (ty == ofbx::Object::Type::VIDEO) {
      const ofbx::Video *v = static_cast<const ofbx::Video*>(o);
      std::string p = DataViewStr(v->getRelativeFileName());
      if (p.empty()) {
        p = DataViewStr(v->getFileName());
      }
      if (p.empty()) {
        p = o->name;
      }
      if (!p.empty()) {
        clips->push_back(p);
      }
    } else if (ty == ofbx::Object::Type::LAYERED_TEXTURE) {
      std::string p;
      for (int j = 0; j < 8; ++j) {
        ofbx::Object *inner = const_cast<ofbx::Object*>(o)->resolveObjectLink(j);
        if (!inner) {
          break;
        }
        if (inner->getType() == ofbx::Object::Type::TEXTURE) {
          p = TexturePathOf(static_cast<const ofbx::Texture*>(inner));
          if (!p.empty()) {
            break;
          }
        }
      }
      if (!p.empty()) {
        clips->push_back(p);
      }
    }
  }
}

std::string ResolveNamed(const std::string &raw, const std::string &mat_name,
    const FbxLoadOptions &opt) {
  std::string path = raw;
  if (opt.remap) {
    std::string remapped = opt.remap(raw, mat_name, opt.remap_user);
    if (!remapped.empty()) {
      path = remapped;
    }
  }
  if (path.empty()) {
    return std::string();
  }
  return ResolveAssetPath(path, opt.search_dirs);
}

struct AccumVert {
  Vec3F p = Vec3F(0.0f, 0.0f, 0.0f);
  Vec3F n = Vec3F(0.0f, 1.0f, 0.0f);
  Vec2F uv = Vec2F(0.0f, 0.0f);
  Vec2F uv1 = Vec2F(0.0f, 0.0f);
  Vec2F uv2 = Vec2F(0.0f, 0.0f);
  Vec2F uv3 = Vec2F(0.0f, 0.0f);
  Vec3F tan = Vec3F(0.0f, 0.0f, 0.0f);
  float cr = 1.0f;
  float cg = 1.0f;
  float cb = 1.0f;
  float ca = 1.0f;
  std::vector<SkinInfluence> skin;
};

struct AccumPart {
  std::vector<AccumVert> verts;
  std::string raw_tex;
  std::string raw_nml;
  std::string node_name;
  std::string material_name;
  Vec3F color;
  bool has_color = false;
  bool has_tan = false;
  bool has_uv1 = false;
  bool has_uv2 = false;
  bool has_uv3 = false;
};

std::string PartKey(const std::string &tex, const std::string &mat_name,
    int mesh_id, bool skinned) {
  std::ostringstream os;
  os << (tex.empty() ? "ntex" : tex) << "|"
      << (mat_name.empty() ? "nmat" : mat_name);
  if (skinned) {
    os << "|m" << mesh_id;
  }
  return os.str();
}

Si32 AddBone(Model *out, const ofbx::Object *obj, bool z_up, float unit,
    std::map<const ofbx::Object*, Si32> *index,
    Si32 *missing_link) {
  if (!obj) {
    if (missing_link) {
      ++(*missing_link);
    }
    return -1;
  }
  std::map<const ofbx::Object*, Si32>::iterator it = index->find(obj);
  if (it != index->end()) {
    return it->second;
  }
  Si32 parent = -1;
  const ofbx::Object *p = obj->getParent();
  if (p && p != obj) {
    parent = AddBone(out, p, z_up, unit, index, missing_link);
  }
  Bone bone;
  bone.name = obj->name;
  bone.parent = parent;
  bone.local_bind = OfbxToMat44(obj->getLocalTransform(), z_up);
  bone.inverse_bind = Identity44();
  if (unit != 1.0f) {
    bone.local_bind.m[3] *= unit;
    bone.local_bind.m[7] *= unit;
    bone.local_bind.m[11] *= unit;
  }
  Si32 id = static_cast<Si32>(out->bones.size());
  out->bones.push_back(bone);
  (*index)[obj] = id;
  return id;
}

bool FlushAccum(const AccumPart &acc, Model *out, const FbxLoadOptions &opt,
    GlTextureCache *cache) {
  Si32 nvert = static_cast<Si32>(acc.verts.size());
  if (nvert < 3) {
    return false;
  }
  Si32 faces = nvert / 3;
  MeshVertexFormat fmt0 = MeshFormatPosNormUv();
  MeshVertexFormat extra;
  bool use_extra = acc.has_color || acc.has_tan || acc.has_uv1 ||
      acc.has_uv2 || acc.has_uv3;
  if (use_extra) {
    extra.AddElement("vColor", 4, kRMVEDT_Float, false);
    extra.AddElement("vTangent", 3, kRMVEDT_Float, false);
    extra.AddElement("vTexCoord1", 2, kRMVEDT_Float, false);
    extra.AddElement("vTexCoord2", 2, kRMVEDT_Float, false);
    extra.AddElement("vTexCoord3", 2, kRMVEDT_Float, false);
  }
  MeshVertexFormat fmts[2];
  fmts[0] = fmt0;
  fmts[1] = extra;
  std::unique_ptr<Mesh> mesh(new Mesh());
  if (!mesh->Init(use_extra ? 2 : 1, nvert, fmts, kRMVEDT_Polys, 1, faces)) {
    *Log() << "Mesh init failed for " << acc.node_name;
    return false;
  }
  for (Si32 i = 0; i < nvert; ++i) {
    const AccumVert &v = acc.verts[static_cast<size_t>(i)];
    mesh->AddVertex(0, v.p.x, v.p.y, v.p.z, v.n.x, v.n.y, v.n.z,
        v.uv.x, v.uv.y);
    if (use_extra) {
      mesh->AddVertex(1, v.cr, v.cg, v.cb, v.ca, v.tan.x, v.tan.y, v.tan.z,
          v.uv1.x, v.uv1.y, v.uv2.x, v.uv2.y, v.uv3.x, v.uv3.y);
    }
  }
  for (Si32 f = 0; f < faces; ++f) {
    mesh->AddFace(0, f * 3, f * 3 + 1, f * 3 + 2);
  }
  mesh->CalcBBox(0, 0);

  ModelPart part;
  part.mesh = std::move(mesh);
  part.raw_tex = acc.raw_tex;
  part.raw_normal = acc.raw_nml;
  part.node_name = acc.node_name;
  part.material_name = acc.material_name;
  part.color = acc.color;
  part.resolved_tex = ResolveNamed(acc.raw_tex, acc.material_name, opt);
  part.resolved_normal = ResolveNamed(acc.raw_nml, acc.material_name, opt);
  if (opt.upload_textures && cache) {
    if (!part.resolved_tex.empty()) {
      GlTextureCacheEntry e = cache->Load(part.resolved_tex.c_str());
      part.diffuse = e.texture;
      part.has_alpha = e.has_alpha;
    }
    if (!part.resolved_normal.empty()) {
      GlTextureCacheEntry e = cache->Load(part.resolved_normal.c_str());
      part.normal = e.texture;
    }
  }
  if (part.diffuse) {
    part.color = Vec3F(1.0f, 1.0f, 1.0f);
  }
  part.skin.resize(static_cast<size_t>(nvert));
  for (Si32 i = 0; i < nvert; ++i) {
    part.skin[static_cast<size_t>(i)] = acc.verts[static_cast<size_t>(i)].skin;
  }
  out->parts.push_back(std::move(part));
  return true;
}

}  // namespace

bool LoadFbx(Model *out, const Ui8 *data, Si32 size,
    const FbxLoadOptions &opt) {
  if (!out) {
    return false;
  }
  out->parts.clear();
  out->bones.clear();
  out->clips.clear();
  out->takes.clear();
  out->texture_clips.clear();
  out->z_up = false;
  out->unit_scale = 1.0f;
  if (!data || size <= 0) {
    return false;
  }
  ofbx::IScene *scene = ofbx::load(data, size);
  if (!scene) {
    *Log() << "ofbx::load failed: " << ofbx::getError();
    return false;
  }
  const ofbx::GlobalSettings *gs = scene->getGlobalSettings();
  bool z_up = false;
  float unit = 1.0f;
  if (gs) {
    z_up = (gs->UpAxis == ofbx::UpVector_AxisZ);
    unit = gs->UnitScaleFactor;
    if (unit <= 0.0f) {
      unit = 1.0f;
    }
    *Log() << "FBX meshes=" << scene->getMeshCount()
      << " up=" << static_cast<int>(gs->UpAxis)
      << " unit=" << unit;
  } else {
    *Log() << "FBX meshes=" << scene->getMeshCount();
  }
  if (!opt.apply_unit_scale) {
    unit = 1.0f;
  }
  out->z_up = z_up;
  out->unit_scale = unit;

  GlTextureCache *cache = opt.cache;
  if (opt.upload_textures && cache == nullptr) {
    out->cache.reset(new GlTextureCache());
    cache = out->cache.get();
  }

  CatalogClips(scene, &out->texture_clips);

  std::map<const ofbx::Object*, Si32> bone_index;
  Si32 skipped = 0;
  Si32 missing_link = 0;
  std::map<std::string, AccumPart> accums;
  int mesh_count = scene->getMeshCount();
  Si32 tri_total = 0;
  for (int mi = 0; mi < mesh_count; ++mi) {
    const ofbx::Mesh *om = scene->getMesh(mi);
    if (!om) {
      ++skipped;
      continue;
    }
    const ofbx::Geometry *geom = om->getGeometry();
    if (!geom) {
      ++skipped;
      continue;
    }
    const Vec3D *verts = geom->getVertices();
    int nvert = geom->getVertexCount();
    if (!verts || nvert < 3) {
      ++skipped;
      continue;
    }
    const Vec3D *norms = geom->getNormals();
    const Vec2D *uvs = geom->getUVs(0);
    const Vec2D *uvs1 = geom->getUVs(1);
    const Vec2D *uvs2 = geom->getUVs(2);
    const Vec2D *uvs3 = geom->getUVs(3);
    const ofbx::Vec4D *colors = geom->getColors();
    const Vec3D *tans = geom->getTangents();
    const int *mats = geom->getMaterials();
    ofbx::Matrix44D world = Mul44(om->getGlobalTransform(),
        om->getGeometricMatrix());
    const ofbx::Skin *skin = geom->getSkin();
    std::vector<std::vector<SkinInfluence> > geom_skin;
    if (skin) {
      geom_skin.resize(static_cast<size_t>(nvert));
      int nc = skin->getClusterCount();
      for (int ci = 0; ci < nc; ++ci) {
        const ofbx::Cluster *cl = skin->getCluster(ci);
        if (!cl) {
          continue;
        }
        const ofbx::Object *link = cl->getLink();
        Si32 bone = AddBone(out, link, z_up, unit, &bone_index, &missing_link);
        if (bone < 0) {
          continue;
        }
        out->bones[static_cast<size_t>(bone)].inverse_bind =
            OfbxToMat44(cl->getTransformLinkMatrix(), z_up);
        if (unit != 1.0f) {
          Mat44F &ib = out->bones[static_cast<size_t>(bone)].inverse_bind;
          ib.m[3] *= unit;
          ib.m[7] *= unit;
          ib.m[11] *= unit;
        }
        const int *idx = cl->getIndices();
        const double *w = cl->getWeights();
        int nw = cl->getIndicesCount();
        if (!idx || !w || nw != cl->getWeightsCount()) {
          continue;
        }
        for (int k = 0; k < nw; ++k) {
          int vi = idx[k];
          if (vi < 0 || vi >= nvert) {
            continue;
          }
          SkinInfluence inf;
          inf.bone = bone;
          inf.weight = static_cast<float>(w[k]);
          geom_skin[static_cast<size_t>(vi)].push_back(inf);
        }
      }
    }
    int ntri = nvert / 3;
    tri_total += ntri;
    for (int t = 0; t < ntri; ++t) {
      int mat_idx = 0;
      if (mats) {
        mat_idx = mats[t];
      }
      if (mat_idx < 0) {
        mat_idx = 0;
      }
      Vec3F color(0.72f, 0.74f, 0.78f);
      std::string tex;
      std::string nml;
      std::string mat_name;
      if (mat_idx < om->getMaterialCount()) {
        const ofbx::Material *mat = om->getMaterial(mat_idx);
        if (mat) {
          ofbx::RgbF dc = mat->getDiffuseColor();
          color = Vec3F(dc.r, dc.g, dc.b);
          tex = ResolveMaterialTex(mat, ofbx::Texture::DIFFUSE);
          nml = ResolveMaterialTex(mat, ofbx::Texture::NORMAL);
          mat_name = mat->name;
        }
      }
      std::string key = PartKey(tex, mat_name, mi, skin != nullptr);
      AccumPart &acc = accums[key];
      if (acc.verts.empty()) {
        acc.raw_tex = tex;
        acc.raw_nml = nml;
        acc.node_name = om->name;
        acc.material_name = mat_name;
        acc.color = color;
      }
      AccumVert tv[3];
      for (int k = 0; k < 3; ++k) {
        int i = t * 3 + k;
        tv[k].p = MaybeZup(XformPoint(world, verts[i]), z_up) * unit;
        if (norms) {
          tv[k].n = MaybeZup(XformDir(world, norms[i]), z_up);
        } else {
          tv[k].n = Vec3F(0.0f, 1.0f, 0.0f);
        }
        if (uvs) {
          tv[k].uv = Vec2F(static_cast<float>(uvs[i].x),
              static_cast<float>(uvs[i].y));
        }
        if (uvs1) {
          tv[k].uv1 = Vec2F(static_cast<float>(uvs1[i].x),
              static_cast<float>(uvs1[i].y));
          acc.has_uv1 = true;
        }
        if (uvs2) {
          tv[k].uv2 = Vec2F(static_cast<float>(uvs2[i].x),
              static_cast<float>(uvs2[i].y));
          acc.has_uv2 = true;
        }
        if (uvs3) {
          tv[k].uv3 = Vec2F(static_cast<float>(uvs3[i].x),
              static_cast<float>(uvs3[i].y));
          acc.has_uv3 = true;
        }
        if (colors) {
          tv[k].cr = static_cast<float>(colors[i].x);
          tv[k].cg = static_cast<float>(colors[i].y);
          tv[k].cb = static_cast<float>(colors[i].z);
          tv[k].ca = static_cast<float>(colors[i].w);
          acc.has_color = true;
        }
        if (tans) {
          tv[k].tan = MaybeZup(XformDir(world, tans[i]), z_up);
          acc.has_tan = true;
        }
        if (!geom_skin.empty()) {
          tv[k].skin = geom_skin[static_cast<size_t>(i)];
        }
      }
      if (!norms) {
        Vec3F fn = Cross(tv[1].p - tv[0].p, tv[2].p - tv[0].p);
        float len = Length(fn);
        if (len > 1e-8f) {
          fn = fn / len;
        } else {
          fn = Vec3F(0.0f, 1.0f, 0.0f);
        }
        tv[0].n = tv[1].n = tv[2].n = fn;
      } else {
        for (int k = 0; k < 3; ++k) {
          float len = Length(tv[k].n);
          if (len > 1e-8f) {
            tv[k].n = tv[k].n / len;
          }
        }
      }
      acc.verts.push_back(tv[0]);
      acc.verts.push_back(tv[1]);
      acc.verts.push_back(tv[2]);
    }
  }

  Si32 built = 0;
  for (std::map<std::string, AccumPart>::iterator it = accums.begin();
      it != accums.end(); ++it) {
    if (FlushAccum(it->second, out, opt, cache)) {
      ++built;
    }
  }

  int nstack = scene->getAnimationStackCount();
  for (int si = 0; si < nstack; ++si) {
    const ofbx::AnimationStack *stack = scene->getAnimationStack(si);
    if (!stack) {
      continue;
    }
    AnimClip clip;
    clip.name = stack->name;
    for (int li = 0; li < 32; ++li) {
      const ofbx::AnimationLayer *layer = stack->getLayer(li);
      if (!layer) {
        break;
      }
      for (int ni = 0; ni < 1024; ++ni) {
        const ofbx::AnimationCurveNode *node = layer->getCurveNode(ni);
        if (!node) {
          break;
        }
        const ofbx::Object *bone_obj = node->getBone();
        Si32 bone = -1;
        if (bone_obj) {
          bone = AddBone(out, bone_obj, z_up, unit, &bone_index, nullptr);
        } else {
          *Log() << "Animation curve without a bone on " << clip.name;
        }
        std::string prop = DataViewStr(node->getLinkProperty());
        for (int axis = 0; axis < 3; ++axis) {
          const ofbx::AnimationCurve *curve = node->getCurve(axis);
          if (!curve) {
            continue;
          }
          AnimCurve ac;
          ac.bone = bone;
          ac.property = prop;
          ac.axis = axis;
          int nk = curve->getKeyCount();
          const Si64 *times = curve->getKeyTime();
          const float *values = curve->getKeyValue();
          if (!times || !values || nk <= 0) {
            continue;
          }
          ac.times.assign(times, times + nk);
          ac.values.assign(values, values + nk);
          clip.curves.push_back(ac);
        }
      }
    }
    if (!clip.curves.empty() || !clip.name.empty()) {
      out->clips.push_back(clip);
    }
  }

  int ntake = scene->getTakeCount();
  for (int ti = 0; ti < ntake; ++ti) {
    const ofbx::TakeInfo *info = scene->getTake(ti);
    if (!info) {
      continue;
    }
    AnimTake take;
    take.name = DataViewStr(info->name);
    take.local_from = info->local_time_from;
    take.local_to = info->local_time_to;
    out->takes.push_back(take);
  }

  scene->destroy();
  *Log() << "FBX triangles " << tri_total << " parts " << built
    << " skipped_empty=" << skipped
    << " clusters_without_link=" << missing_link
    << " bones=" << out->bones.size()
    << " clips=" << out->clips.size();
  return built > 0;
}

bool LoadFbx(Model *out, const char *path, const FbxLoadOptions &opt) {
  if (!path) {
    return false;
  }
  std::vector<Ui8> data = ReadFile(path, true);
  if (data.empty()) {
    *Log() << "FBX missing: " << path;
    return false;
  }
  *Log() << "FBX " << path;
  return LoadFbx(out, data.data(), static_cast<Si32>(data.size()), opt);
}

}  // namespace arctic
