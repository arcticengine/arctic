// Meshes and models: the Mesh container, PLY, FBX through ofbx and LoadFbx,
// asset path resolution. The FbxNode helpers build binary FBX files in memory.
#define TEST_NO_MAIN
#include "test_helpers.h"

// ---------------------------------------------------------------------------
// Mesh bug regression tests
// ---------------------------------------------------------------------------

// Reproduce the readline() bug from mesh_ply.cpp: the while-loop that strips
// trailing CR/LF never decrements 'l', so only the last character is removed.
// On files with \r\n line endings, \r survives and all strcmp() comparisons
// in the PLY parser fail.
static int ply_readline_fixed(char *str, int max, const char *src) {
  strncpy(str, src, (size_t)max);
  str[max - 1] = 0;
  size_t l = strlen(str);
  while (l > 0 && (str[l - 1] == 10 || str[l - 1] == 13)) {
    str[l - 1] = 0;
    l--;
  }
  return 1;
}

void test_mesh_ply_readline_crlf(void) {
  char buf[256];

  // Unix line ending: should strip \n, result is "ply"
  ply_readline_fixed(buf, 256, "ply\n");
  TEST_CHECK(strcmp(buf, "ply") == 0);
  TEST_MSG("Unix ending: got \"%s\"", buf);

  // Windows line ending: should strip \r\n, result should be "ply"
  ply_readline_fixed(buf, 256, "ply\r\n");
  TEST_CHECK(strcmp(buf, "ply") == 0);
  TEST_MSG("Windows ending: got \"%s\" (len=%zu), expected \"ply\" (len=3)",
           buf, strlen(buf));
}

// Reproduce the mesh_ply.cpp bug: vertex format declares element 1 as 2 floats,
// but 3 floats are written, overwriting vertex (i+1)'s position data.
void test_mesh_vertex_attrib_write_overflow(void) {
  Mesh mesh;

  // Same format as mesh_ply.cpp:
  // element 0: 3 floats (position), element 1: 3 floats (normal)
  // stride = 6 * sizeof(float) = 24 bytes
  const MeshVertexFormat vf = {6 * (int)sizeof(float), 2, 0,
    {{3, kRMVEDT_Float, false}, {3, kRMVEDT_Float, false}}};

  bool ok = mesh.Init(1, 4, &vf, kRMVEDT_Polys, 1, 1);
  TEST_CHECK(ok);
  if (!ok) {
    return;
  }

  // Write known position values for vertex 0 and vertex 1
  float *v0 = (float *)mesh.GetVertexData(0, 0, 0);  // vertex 0, element 0 (pos)
  float *v1 = (float *)mesh.GetVertexData(0, 1, 0);  // vertex 1, element 0 (pos)
  v0[0] = 1.0f;  v0[1] = 2.0f;  v0[2] = 3.0f;
  v1[0] = 10.0f; v1[1] = 20.0f; v1[2] = 30.0f;

  // Now write to element 1 of vertex 0 (texcoord, 2 floats)
  float *nt = (float *)mesh.GetVertexData(0, 0, 1);  // vertex 0, element 1

  // Correct: write only 2 floats (within the 2-float attribute)
  nt[0] = 100.0f;
  nt[1] = 200.0f;

  // Verify vertex 1 position is intact after writing 2 floats
  TEST_CHECK(v1[0] == 10.0f);
  TEST_MSG("After 2-float write: v1[0] = %f, expected 10.0", (double)v1[0]);

  // Reproduce mesh_ply.cpp pattern: write 3 floats into a 2-float attribute.
  // The vertex format must have room for 3 floats in element 1 for this to
  // be safe.  If it only has 2, nt[2] overwrites the next vertex.
  nt[0] = 100.0f;
  nt[1] = 200.0f;
  nt[2] = 300.0f;  // Would overwrite v1[0] if element 1 is only 2 floats

  TEST_CHECK(v1[0] == 10.0f);
  TEST_MSG("After 3-float write into element 1: v1[0] = %f, expected 10.0",
           (double)v1[0]);
}

// Test Mesh_ExtrudeFace: extrusion of a single triangle should create 3 side
// quads (6 triangles) connecting original and extruded vertices.  With the
// i+=2 bug, only edges 0 and 2 are processed, edge 1 is skipped.
void test_mesh_extrude_face_covers_all_edges(void) {
  Mesh mesh;

  // Simple format: 3 floats per vertex (position only)
  const MeshVertexFormat vf = {3 * (int)sizeof(float), 1, 0,
    {{3, kRMVEDT_Float, false}}};

  // Allocate room for 6 verts (3 original + 3 extruded) and
  // 7 faces (1 original + 6 side faces = 3 quads * 2 triangles each).
  // We over-allocate so we can check what Mesh_ExtrudeFace actually writes.
  bool ok = mesh.Init(1, 128, &vf, kRMVEDT_Polys, 1, 128);
  TEST_CHECK(ok);
  if (!ok) {
    return;
  }

  // Set up a single triangle: vertices 0, 1, 2
  float *v0 = (float *)mesh.GetVertexData(0, 0, 0);
  float *v1 = (float *)mesh.GetVertexData(0, 1, 0);
  float *v2 = (float *)mesh.GetVertexData(0, 2, 0);
  v0[0] = 0.0f; v0[1] = 0.0f; v0[2] = 0.0f;
  v1[0] = 1.0f; v1[1] = 0.0f; v1[2] = 0.0f;
  v2[0] = 0.0f; v2[1] = 1.0f; v2[2] = 0.0f;

  mesh.mVertexData.mVertexArray[0].mNum = 3;
  mesh.mFaceData.mIndexArray[0].mBuffer[0].mIndex[0] = 0;
  mesh.mFaceData.mIndexArray[0].mBuffer[0].mIndex[1] = 1;
  mesh.mFaceData.mIndexArray[0].mBuffer[0].mIndex[2] = 2;
  mesh.mFaceData.mIndexArray[0].mNum = 1;

  // Extrude face 0
  ok = Mesh_ExtrudeFace(&mesh, 0);
  TEST_CHECK(ok);
  if (!ok) {
    return;
  }

  // After extrusion we expect:
  //   - 3 new vertices (indices 3, 4, 5)
  //   - 3 new side faces (one per edge of the original triangle)
  //   - The original face now references the new vertices (3, 4, 5)
  TEST_CHECK(mesh.mVertexData.mVertexArray[0].mNum == 6);
  TEST_MSG("Expected 6 vertices, got %u", mesh.mVertexData.mVertexArray[0].mNum);

  // The extrude function adds num=3 to mNum, making total = 1 + 3 = 4.
  // But a correct extrusion of a triangle needs 2*3=6 side faces,
  // so total should be 1 + 6 = 7.
  // With the i+=2 bug, only 4 faces are written but mNum is set to 4.
  Si32 face_count = (Si32)mesh.mFaceData.mIndexArray[0].mNum;
  TEST_MSG("Face count after extrude: %d", face_count);

  // Verify that every original edge (0-1, 1-2, 2-0) is covered by side faces.
  // Original vertex indices are 0, 1, 2; new copies are 3, 4, 5.
  // For edge between original vertex i and i+1 (mod 3), we expect two side
  // triangles connecting (orig_i, orig_j, new_j) and (new_j, new_i, orig_i).
  //
  // Collect all edges from the side faces (faces 1..face_count-1) and check
  // that edges (0,1), (1,2), (2,0) all appear in some side face.
  bool edge01_found = false;
  bool edge12_found = false;
  bool edge20_found = false;

  for (Si32 fi = 1; fi < face_count; ++fi) {
    MeshFace *f = &mesh.mFaceData.mIndexArray[0].mBuffer[fi];
    for (int e = 0; e < 3; ++e) {
      int a = f->mIndex[e];
      int b = f->mIndex[(e + 1) % 3];
      // Check if this edge matches any original edge (in either direction)
      if ((a == 0 && b == 1) || (a == 1 && b == 0)) {
        edge01_found = true;
      }
      if ((a == 1 && b == 2) || (a == 2 && b == 1)) {
        edge12_found = true;
      }
      if ((a == 2 && b == 0) || (a == 0 && b == 2)) {
        edge20_found = true;
      }
    }
  }

  TEST_CHECK(edge01_found);
  TEST_MSG("Edge 0-1 covered by side faces: %s", edge01_found ? "yes" : "NO (BUG: i+=2 skips edge 1)");
  TEST_CHECK(edge12_found);
  TEST_MSG("Edge 1-2 covered by side faces: %s", edge12_found ? "yes" : "NO (BUG: i+=2 skips edge 1)");
  TEST_CHECK(edge20_found);
  TEST_MSG("Edge 2-0 covered by side faces: %s", edge20_found ? "yes" : "NO");
}

// A named element remembers the attribute name for Mesh::Draw, while an unnamed
// one keeps the old positional binding, and both count towards the stride.
void test_mesh_named_elements(void) {
  MeshVertexFormat format;
  int position = format.AddElement("vPosition", 3, kRMVEDT_Float);
  int normal = format.AddElement("vNormal", 3, kRMVEDT_Float);
  int color = format.AddElement(4, kRMVEDT_UByte, true);

  TEST_CHECK_(position == 0 && normal == 1 && color == 2,
      "elements got indices %d, %d, %d instead of 0, 1, 2",
      position, normal, color);
  TEST_CHECK_(format.mElems[position].mName != nullptr
      && strcmp(format.mElems[position].mName, "vPosition") == 0,
      "the name of element 0 was lost");
  TEST_CHECK_(format.mElems[normal].mName != nullptr
      && strcmp(format.mElems[normal].mName, "vNormal") == 0,
      "the name of element 1 was lost");
  TEST_CHECK_(format.mElems[color].mName == nullptr,
      "an unnamed element got a name out of nowhere");
  TEST_CHECK_(format.mElems[normal].mOffset == 3 * sizeof(float),
      "element 1 sits at offset %u instead of %u",
      format.mElems[normal].mOffset,
      (unsigned int)(3 * sizeof(float)));
  TEST_CHECK_(format.mStride == (int)(6 * sizeof(float) + 4),
      "the stride is %d instead of %d", format.mStride,
      (int)(6 * sizeof(float) + 4));
}

// Init fixes the capacity: AddVertex and AddFace refuse to write past it and
// say so with -1 instead of growing or overflowing the buffer.
void test_mesh_capacity_is_final(void) {
  MeshVertexFormat format;
  format.AddElement("vPosition", 3, kRMVEDT_Float);

  Mesh mesh;
  TEST_CHECK(mesh.Init(1, 3, &format, kRMVEDT_Polys, 1, 1));

  for (int i = 0; i < 3; ++i) {
    int id = mesh.AddVertex(0, 0.0f, 0.0f, (float)i);
    TEST_CHECK_(id == i, "vertex %d got index %d", i, id);
  }
  TEST_CHECK_(mesh.AddFace(0, 0, 1, 2) == 0, "the only face was refused");

  TEST_CHECK_(mesh.AddVertex(0, 1.0f, 1.0f, 1.0f) == -1,
      "a vertex past the capacity was accepted");
  TEST_CHECK_(mesh.AddFace(0, 0, 1, 2) == -1,
      "a face past the capacity was accepted");
  TEST_CHECK_(mesh.GetCurrentVertexCount(0) == 3,
      "the vertex count moved to %d", mesh.GetCurrentVertexCount(0));
  TEST_CHECK_(mesh.GetCurrentFaceCount(0) == 1,
      "the face count moved to %d", mesh.GetCurrentFaceCount(0));

  // Expand is the way to get more room, and it has to serve every stream and
  // every index array, not only the first one.
  TEST_CHECK(mesh.Expand(64, 64));
  TEST_CHECK_(mesh.AddVertex(0, 1.0f, 1.0f, 1.0f) == 3,
      "the vertex after Expand was still refused");
  TEST_CHECK_(mesh.AddFace(0, 1, 2, 3) == 1,
      "the face after Expand was still refused");
}

void test_mesh_expand_every_stream(void) {
  MeshVertexFormat format[2];
  format[0].AddElement("vPosition", 3, kRMVEDT_Float);
  format[1].AddElement("vTexCoord", 2, kRMVEDT_Float);

  Mesh mesh;
  TEST_CHECK(mesh.Init(2, 4, format, kRMVEDT_Polys, 2, 2));

  const unsigned int vertex_max_before = mesh.mVertexData.mVertexArray[1].mMax;
  const unsigned int face_max_before = mesh.mFaceData.mIndexArray[1].mMax;
  TEST_CHECK(mesh.Expand(64, 64));
  TEST_CHECK_(mesh.mVertexData.mVertexArray[1].mMax > vertex_max_before,
      "the second vertex stream stayed at %u vertices", vertex_max_before);
  TEST_CHECK_(mesh.mFaceData.mIndexArray[1].mMax > face_max_before,
      "the second index array stayed at %u faces", face_max_before);
}

// Clone() has to carry the vertex and face counts over, not only the
// buffers: Init() on the destination zeroes mNum, and a clone that reports
// zero vertices is an empty mesh to everything that draws or measures it.
void test_mesh_clone_keeps_counts_and_data() {
  MeshVertexFormat format;
  format.AddElement("vPosition", 3, kRMVEDT_Float);

  Mesh source;
  TEST_CHECK(source.Init(1, 8, &format, kRMVEDT_Polys, 1, 4));
  source.AddVertex(0, 0.0f, 0.0f, 0.0f);
  source.AddVertex(0, 1.0f, 0.0f, 0.0f);
  source.AddVertex(0, 0.0f, 1.0f, 0.0f);
  source.AddVertex(0, 0.0f, 0.0f, 1.0f);
  source.AddFace(0, 0, 1, 2);
  source.AddFace(0, 0, 2, 3);
  TEST_CHECK(source.GetCurrentVertexCount(0) == 4);
  TEST_CHECK(source.GetCurrentFaceCount(0) == 2);

  Mesh clone;
  TEST_CHECK(source.Clone(&clone));
  TEST_CHECK_(clone.GetCurrentVertexCount(0) == 4,
      "the clone reports %d vertices instead of 4",
      clone.GetCurrentVertexCount(0));
  TEST_CHECK_(clone.GetCurrentFaceCount(0) == 2,
      "the clone reports %d faces instead of 2",
      clone.GetCurrentFaceCount(0));

  // The counts alone could be copied over an empty buffer, so read the
  // geometry back as well.
  const float *source_xyz = static_cast<const float*>(
      source.GetVertexData(0, 3, 0));
  const float *clone_xyz = static_cast<const float*>(
      clone.GetVertexData(0, 3, 0));
  TEST_CHECK_(source_xyz != nullptr && clone_xyz != nullptr,
      "no vertex data to compare");
  if (source_xyz != nullptr && clone_xyz != nullptr) {
    TEST_CHECK_(clone_xyz[0] == 0.0f && clone_xyz[1] == 0.0f
            && clone_xyz[2] == 1.0f,
        "the fourth vertex of the clone is (%g, %g, %g), not (0, 0, 1)",
        clone_xyz[0], clone_xyz[1], clone_xyz[2]);
  }
  const MeshFace *clone_faces = clone.mFaceData.mIndexArray[0].mBuffer;
  TEST_CHECK_(clone_faces != nullptr, "no face data to compare");
  if (clone_faces != nullptr) {
    const MeshFace &face = clone_faces[1];
    TEST_CHECK_(face.mIndex[0] == 0 && face.mIndex[1] == 2
            && face.mIndex[2] == 3,
        "the second face of the clone is (%d, %d, %d), not (0, 2, 3)",
        face.mIndex[0], face.mIndex[1], face.mIndex[2]);
  }
}

// ============================================================================
// FBX tests
// ============================================================================

// Every record of a binary FBX stores the absolute offset of its own end, so
// the tree has to be complete before the offsets can be filled in. A scene is
// collected as FbxNode objects here and serialized by FbxBuild afterwards.
struct FbxNode {
  explicit FbxNode(const char *node_name)
      : name(node_name) {
  }

  FbxNode &Long(Si64 value) {
    props.push_back(static_cast<Ui8>('L'));
    AppendBytes(&value, sizeof(value));
    ++prop_count;
    return *this;
  }

  FbxNode &Int(Si32 value) {
    props.push_back(static_cast<Ui8>('I'));
    AppendBytes(&value, sizeof(value));
    ++prop_count;
    return *this;
  }

  FbxNode &Double(double value) {
    props.push_back(static_cast<Ui8>('D'));
    AppendBytes(&value, sizeof(value));
    ++prop_count;
    return *this;
  }

  FbxNode &Str(const char *value) {
    props.push_back(static_cast<Ui8>('S'));
    Ui32 length = static_cast<Ui32>(strlen(value));
    AppendBytes(&length, sizeof(length));
    AppendBytes(value, length);
    ++prop_count;
    return *this;
  }

  FbxNode &DoubleArray(const std::vector<double> &values) {
    props.push_back(static_cast<Ui8>('d'));
    AppendArrayHeader(static_cast<Ui32>(values.size()),
        static_cast<Ui32>(values.size() * sizeof(double)));
    AppendBytes(values.data(), values.size() * sizeof(double));
    ++prop_count;
    return *this;
  }

  FbxNode &IntArray(const std::vector<Si32> &values) {
    props.push_back(static_cast<Ui8>('i'));
    AppendArrayHeader(static_cast<Ui32>(values.size()),
        static_cast<Ui32>(values.size() * sizeof(Si32)));
    AppendBytes(values.data(), values.size() * sizeof(Si32));
    ++prop_count;
    return *this;
  }

  FbxNode &LongArray(const std::vector<Si64> &values) {
    props.push_back(static_cast<Ui8>('l'));
    AppendArrayHeader(static_cast<Ui32>(values.size()),
        static_cast<Ui32>(values.size() * sizeof(Si64)));
    AppendBytes(values.data(), values.size() * sizeof(Si64));
    ++prop_count;
    return *this;
  }

  FbxNode &FloatArray(const std::vector<float> &values) {
    props.push_back(static_cast<Ui8>('f'));
    AppendArrayHeader(static_cast<Ui32>(values.size()),
        static_cast<Ui32>(values.size() * sizeof(float)));
    AppendBytes(values.data(), values.size() * sizeof(float));
    ++prop_count;
    return *this;
  }

  FbxNode &Child(const FbxNode &child) {
    children.push_back(child);
    return *this;
  }

  void AppendBytes(const void *data, std::size_t size) {
    if (size == 0) {
      return;
    }
    const Ui8 *bytes = static_cast<const Ui8*>(data);
    props.insert(props.end(), bytes, bytes + size);
  }

  void AppendArrayHeader(Ui32 count, Ui32 byte_count) {
    Ui32 encoding = 0;
    AppendBytes(&count, sizeof(count));
    AppendBytes(&encoding, sizeof(encoding));
    AppendBytes(&byte_count, sizeof(byte_count));
  }

  std::string name;
  std::vector<Ui8> props;
  Ui32 prop_count = 0;
  std::vector<FbxNode> children;
};

// A record that has children is closed by an all-zero record of its own, and
// so is the list of top level records.
static const int kFbxSentinelSize = 13;

static const char kFbxMagic[] = "Kaydara FBX Binary  ";

void FbxAppendUi32(std::vector<Ui8> *out, Ui32 value) {
  const Ui8 *bytes = reinterpret_cast<const Ui8*>(&value);
  out->insert(out->end(), bytes, bytes + sizeof(value));
}

void FbxAppendHeader(std::vector<Ui8> *out, Ui32 version) {
  out->insert(out->end(), kFbxMagic, kFbxMagic + sizeof(kFbxMagic));
  out->push_back(0x1a);
  out->push_back(0x00);
  FbxAppendUi32(out, version);
}

void FbxWriteNode(const FbxNode &node, std::vector<Ui8> *out) {
  const std::size_t end_offset_pos = out->size();
  FbxAppendUi32(out, 0);
  FbxAppendUi32(out, node.prop_count);
  FbxAppendUi32(out, static_cast<Ui32>(node.props.size()));
  out->push_back(static_cast<Ui8>(node.name.size()));
  out->insert(out->end(), node.name.begin(), node.name.end());
  out->insert(out->end(), node.props.begin(), node.props.end());
  if (!node.children.empty()) {
    for (std::size_t i = 0; i < node.children.size(); ++i) {
      FbxWriteNode(node.children[i], out);
    }
    out->insert(out->end(), kFbxSentinelSize, 0);
  }
  const Ui32 end_offset = static_cast<Ui32>(out->size());
  memcpy(&(*out)[end_offset_pos], &end_offset, sizeof(end_offset));
}

std::vector<Ui8> FbxBuild(const std::vector<FbxNode> &roots, Ui32 version) {
  std::vector<Ui8> out;
  FbxAppendHeader(&out, version);
  for (std::size_t i = 0; i < roots.size(); ++i) {
    FbxWriteNode(roots[i], &out);
  }
  out.insert(out.end(), kFbxSentinelSize, 0);
  return out;
}

std::vector<Ui8> FbxBuildScene(const FbxNode &objects,
    const FbxNode &connections) {
  std::vector<FbxNode> roots;
  roots.push_back(objects);
  roots.push_back(connections);
  return FbxBuild(roots, 7400);
}

// One entry of a Properties70 block: a name, the three type strings the
// format carries along, and the value as property number four.
FbxNode FbxIntProperty(const char *name, Si32 value) {
  FbxNode property("P");
  property.Str(name).Str("int").Str("Integer").Str("").Int(value);
  return property;
}

FbxNode FbxDoubleProperty(const char *name, double value) {
  FbxNode property("P");
  property.Str(name).Str("double").Str("Number").Str("").Double(value);
  return property;
}

FbxNode FbxGlobalSettingsNode(const FbxNode &properties) {
  FbxNode settings("GlobalSettings");
  settings.Child(properties);
  return settings;
}

// GlobalSettings sits next to Objects and Connections at the top level.
std::vector<Ui8> FbxBuildSceneWithSettings(const FbxNode &settings,
    const FbxNode &objects, const FbxNode &connections) {
  std::vector<FbxNode> roots;
  roots.push_back(settings);
  roots.push_back(objects);
  roots.push_back(connections);
  return FbxBuild(roots, 7400);
}

// Owns the scene so that a failing TEST_CHECK can return without leaking it.
class FbxScene {
 public:
  explicit FbxScene(const std::vector<Ui8> &data)
      : scene_(ofbx::load(&data[0], static_cast<int>(data.size()))) {
  }

  ~FbxScene() {
    if (scene_ != nullptr) {
      scene_->destroy();
    }
  }

  ofbx::IScene *Get() const {
    return scene_;
  }

 private:
  FbxScene(const FbxScene &);
  FbxScene &operator=(const FbxScene &);

  ofbx::IScene *scene_;
};

std::string FbxToString(const ofbx::DataView &view) {
  if (view.begin == nullptr || view.end == nullptr) {
    return std::string();
  }
  return std::string(reinterpret_cast<const char*>(view.begin),
      reinterpret_cast<const char*>(view.end));
}

const ofbx::Object *FbxFindObject(const ofbx::IScene &scene,
    ofbx::Object::Type type) {
  const ofbx::Object *const *objects = scene.getAllObjects();
  for (int i = 0; i < scene.getAllObjectCount(); ++i) {
    if (objects[i] != nullptr && objects[i]->getType() == type) {
      return objects[i];
    }
  }
  return nullptr;
}

FbxNode FbxTextureNode(Si64 id, const char *filename,
    const char *relative_filename) {
  FbxNode texture("Texture");
  texture.Long(id).Str("Texture::tex").Str("");
  if (filename != nullptr) {
    texture.Child(FbxNode("FileName").Str(filename));
  }
  if (relative_filename != nullptr) {
    texture.Child(FbxNode("RelativeFilename").Str(relative_filename));
  }
  return texture;
}

FbxNode FbxVideoNode(Si64 id, const char *filename_element,
    const char *filename, const char *relative_filename) {
  FbxNode video("Video");
  video.Long(id).Str("Video::clip").Str("Clip");
  video.Child(FbxNode(filename_element).Str(filename));
  if (relative_filename != nullptr) {
    video.Child(FbxNode("RelativeFilename").Str(relative_filename));
  }
  return video;
}

FbxNode FbxMaterialNode(Si64 id) {
  FbxNode material("Material");
  material.Long(id).Str("Material::mat").Str("");
  return material;
}

FbxNode FbxLayeredTextureNode(Si64 id) {
  FbxNode layered("LayeredTexture");
  layered.Long(id).Str("LayeredTexture::layered").Str("");
  return layered;
}

FbxNode FbxConnectOO(Si64 from, Si64 to) {
  FbxNode connection("C");
  connection.Str("OO").Long(from).Long(to);
  return connection;
}

FbxNode FbxConnectOP(Si64 from, Si64 to, const char *property) {
  FbxNode connection("C");
  connection.Str("OP").Long(from).Long(to).Str(property);
  return connection;
}

// Builds a scene of one Material with one Texture bound to it through the
// given connection property, and returns the texture the material resolved
// for the given slot, or an empty string when the slot stayed empty.
std::string FbxResolveMaterialTexture(const char *connection_property,
    ofbx::Texture::TextureType slot) {
  const Si64 kMaterialId = 100;
  const Si64 kTextureId = 200;

  FbxNode objects("Objects");
  objects.Child(FbxMaterialNode(kMaterialId));
  objects.Child(FbxTextureNode(kTextureId, "diffuse.png", "tex/diffuse.png"));

  FbxNode connections("Connections");
  connections.Child(FbxConnectOP(kTextureId, kMaterialId,
      connection_property));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (scene.Get() == nullptr) {
    return std::string("<the scene failed to load>");
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::MATERIAL);
  if (object == nullptr) {
    return std::string("<the material is missing>");
  }

  const ofbx::Material *material =
      static_cast<const ofbx::Material*>(object);
  const ofbx::Texture *texture = material->getTexture(slot);
  if (texture == nullptr) {
    return std::string();
  }
  return FbxToString(texture->getFileName());
}

void test_fbx_rejects_unsupported_version() {
  FbxNode objects("Objects");
  objects.Child(FbxTextureNode(100, "diffuse.png", "tex/diffuse.png"));
  FbxNode connections("Connections");

  std::vector<FbxNode> roots;
  roots.push_back(objects);
  roots.push_back(connections);
  FbxScene scene(FbxBuild(roots, 6100));

  if (!TEST_CHECK_(scene.Get() == nullptr,
      "FBX 6.1 is below the supported minimum and must not load")) {
    return;
  }
  TEST_CHECK_(strstr(ofbx::getError(), "version") != nullptr,
      "The error must name the version, got '%s'", ofbx::getError());
}

void test_fbx_rejects_a_buffer_too_short_for_a_header() {
  ofbx::IScene *nothing = ofbx::load(nullptr, 0);
  if (!TEST_CHECK_(nothing == nullptr, "An empty buffer must not load")) {
    nothing->destroy();
    return;
  }

  // One byte short of the 27 byte header, which is read as a whole.
  std::vector<Ui8> truncated;
  FbxAppendHeader(&truncated, 7400);
  truncated.pop_back();
  ofbx::IScene *truncated_scene =
      ofbx::load(&truncated[0], static_cast<int>(truncated.size()));
  if (!TEST_CHECK_(truncated_scene == nullptr,
      "A buffer smaller than the header must not load")) {
    truncated_scene->destroy();
    return;
  }
  TEST_CHECK_(strstr(ofbx::getError(), "too short") != nullptr,
      "The error must say the file is too short, got '%s'", ofbx::getError());

  // Exactly a header and not a single record: long enough to be looked at,
  // too short to hold a scene.
  std::vector<Ui8> header_only;
  FbxAppendHeader(&header_only, 7400);
  ofbx::IScene *header_scene =
      ofbx::load(&header_only[0], static_cast<int>(header_only.size()));
  if (!TEST_CHECK_(header_scene == nullptr,
      "A file of nothing but a header must not load")) {
    header_scene->destroy();
  }
}

void test_fbx_rejects_record_past_end_of_file() {
  std::vector<Ui8> data;
  FbxAppendHeader(&data, 7400);
  FbxAppendUi32(&data, 1000);  // the record claims to end past the file
  FbxAppendUi32(&data, 0);
  FbxAppendUi32(&data, 0);
  data.push_back(200);  // and its name is longer than what is left
  data.push_back(static_cast<Ui8>('X'));

  FbxScene scene(data);
  TEST_CHECK_(scene.Get() == nullptr,
      "A record reaching past the end of the file must not load");
}

void test_fbx_empty_scene_loads() {
  FbxNode objects("Objects");
  FbxNode connections("Connections");

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene without objects must load, got error '%s'", ofbx::getError())) {
    return;
  }
  TEST_CHECK_(scene.Get()->getRoot() != nullptr,
      "Even an empty scene has a root node");
  TEST_CHECK_(scene.Get()->getMeshCount() == 0,
      "A scene without objects has no meshes, got %d",
      scene.Get()->getMeshCount());
  TEST_CHECK_(scene.Get()->getAllObjectCount() == 0,
      "A scene without objects has no objects, got %d",
      scene.Get()->getAllObjectCount());
}

void test_fbx_mesh_geometry_is_triangulated() {
  const Si64 kModelId = 100;
  const Si64 kGeometryId = 200;

  // A unit quad in the z = 0 plane. The last index of a polygon is stored
  // negated and decremented, so 3 becomes -4.
  const double kQuad[12] = {
    0.0, 0.0, 0.0,
    1.0, 0.0, 0.0,
    1.0, 1.0, 0.0,
    0.0, 1.0, 0.0
  };
  const Si32 kPolygon[4] = {0, 1, 2, -4};

  FbxNode geometry("Geometry");
  geometry.Long(kGeometryId).Str("Geometry::quad").Str("Mesh");
  geometry.Child(FbxNode("Vertices").DoubleArray(
      std::vector<double>(kQuad, kQuad + 12)));
  geometry.Child(FbxNode("PolygonVertexIndex").IntArray(
      std::vector<Si32>(kPolygon, kPolygon + 4)));

  FbxNode model("Model");
  model.Long(kModelId).Str("Model::quad").Str("Mesh");

  FbxNode objects("Objects");
  objects.Child(geometry);
  objects.Child(model);

  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeometryId, kModelId));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A mesh scene must load, got error '%s'", ofbx::getError())) {
    return;
  }
  if (!TEST_CHECK_(scene.Get()->getMeshCount() == 1,
      "Expected one mesh, got %d", scene.Get()->getMeshCount())) {
    return;
  }

  const ofbx::Mesh *mesh = scene.Get()->getMesh(0);
  const ofbx::Geometry *geom = mesh->getGeometry();
  if (!TEST_CHECK_(geom != nullptr,
      "The Geometry must be attached to the Model it is connected to")) {
    return;
  }
  if (!TEST_CHECK_(geom->getVertexCount() == 6,
      "A quad must become two triangles, that is 6 vertices, got %d",
      geom->getVertexCount())) {
    return;
  }

  const int kExpected[6] = {0, 1, 2, 0, 2, 3};
  const Vec3D *vertices = geom->getVertices();
  for (int i = 0; i < 6; ++i) {
    const double *expected = kQuad + kExpected[i] * 3;
    TEST_CHECK_(vertices[i].x == expected[0]
        && vertices[i].y == expected[1]
        && vertices[i].z == expected[2],
        "Vertex %d must be (%g, %g, %g), got (%g, %g, %g)",
        i, expected[0], expected[1], expected[2],
        vertices[i].x, vertices[i].y, vertices[i].z);
  }
}

void test_fbx_material_diffuse_color() {
  const Si64 kMaterialId = 100;

  FbxNode diffuse("P");
  diffuse.Str("DiffuseColor").Str("Color").Str("").Str("A");
  diffuse.Double(0.25).Double(0.5).Double(0.75);

  FbxNode properties("Properties70");
  properties.Child(diffuse);

  FbxNode material = FbxMaterialNode(kMaterialId);
  material.Child(properties);

  FbxNode objects("Objects");
  objects.Child(material);
  FbxNode connections("Connections");

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A material scene must load, got error '%s'", ofbx::getError())) {
    return;
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::MATERIAL);
  if (!TEST_CHECK_(object != nullptr, "The Material must be in the scene")) {
    return;
  }

  ofbx::RgbF color =
      static_cast<const ofbx::Material*>(object)->getDiffuseColor();
  TEST_CHECK_(color.r == 0.25f && color.g == 0.5f && color.b == 0.75f,
      "DiffuseColor must be (0.25, 0.5, 0.75), got (%g, %g, %g)",
      color.r, color.g, color.b);
}

void test_fbx_texture_file_names() {
  const Si64 kTextureId = 100;

  FbxNode objects("Objects");
  objects.Child(FbxTextureNode(kTextureId, "C:/art/diffuse.png",
      "tex/diffuse.png"));
  FbxNode connections("Connections");

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A texture scene must load, got error '%s'", ofbx::getError())) {
    return;
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::TEXTURE);
  if (!TEST_CHECK_(object != nullptr, "The Texture must be in the scene")) {
    return;
  }

  const ofbx::Texture *texture = static_cast<const ofbx::Texture*>(object);
  std::string filename = FbxToString(texture->getFileName());
  std::string relative = FbxToString(texture->getRelativeFileName());
  TEST_CHECK_(filename == "C:/art/diffuse.png",
      "FileName must be 'C:/art/diffuse.png', got '%s'", filename.c_str());
  TEST_CHECK_(relative == "tex/diffuse.png",
      "RelativeFilename must be 'tex/diffuse.png', got '%s'",
      relative.c_str());
}

void test_fbx_material_texture_slots() {
  std::string diffuse = FbxResolveMaterialTexture("DiffuseColor",
      ofbx::Texture::DIFFUSE);
  TEST_CHECK_(diffuse == "diffuse.png",
      "A DiffuseColor connection must fill the diffuse slot, got '%s'",
      diffuse.c_str());

  std::string normal = FbxResolveMaterialTexture("NormalMap",
      ofbx::Texture::NORMAL);
  TEST_CHECK_(normal == "diffuse.png",
      "A NormalMap connection must fill the normal slot, got '%s'",
      normal.c_str());

  std::string crossed = FbxResolveMaterialTexture("NormalMap",
      ofbx::Texture::DIFFUSE);
  TEST_CHECK_(crossed.empty(),
      "A NormalMap connection must leave the diffuse slot empty, got '%s'",
      crossed.c_str());
}

void test_fbx_material_texture_qualified_property() {
  std::string qualified = FbxResolveMaterialTexture("Maya|DiffuseColor",
      ofbx::Texture::DIFFUSE);
  TEST_CHECK_(qualified == "diffuse.png",
      "An exporter qualified property must still fill the diffuse slot,"
      " got '%s'", qualified.c_str());

  std::string plain = FbxResolveMaterialTexture("Diffuse",
      ofbx::Texture::DIFFUSE);
  TEST_CHECK_(plain == "diffuse.png",
      "A plain Diffuse property must fill the diffuse slot, got '%s'",
      plain.c_str());
}

void test_fbx_material_ignores_unknown_property() {
  std::string bump = FbxResolveMaterialTexture("Bump",
      ofbx::Texture::DIFFUSE);
  TEST_CHECK_(bump.empty(),
      "A Bump connection belongs to no supported slot, got '%s'",
      bump.c_str());

  std::string emissive = FbxResolveMaterialTexture("EmissiveColor",
      ofbx::Texture::NORMAL);
  TEST_CHECK_(emissive.empty(),
      "An EmissiveColor connection belongs to no supported slot, got '%s'",
      emissive.c_str());
}

void test_fbx_video_supplies_missing_texture_file_name() {
  const Si64 kTextureId = 100;
  const Si64 kVideoId = 200;

  FbxNode objects("Objects");
  objects.Child(FbxTextureNode(kTextureId, nullptr, nullptr));
  objects.Child(FbxVideoNode(kVideoId, "FileName", "C:/art/grass.png",
      "tex/grass.png"));

  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kVideoId, kTextureId));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene with a Video clip must load, got error '%s'",
      ofbx::getError())) {
    return;
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::TEXTURE);
  if (!TEST_CHECK_(object != nullptr, "The Texture must be in the scene")) {
    return;
  }

  const ofbx::Texture *texture = static_cast<const ofbx::Texture*>(object);
  std::string filename = FbxToString(texture->getFileName());
  std::string relative = FbxToString(texture->getRelativeFileName());
  TEST_CHECK_(filename == "C:/art/grass.png",
      "An empty Texture must take FileName from its Video, got '%s'",
      filename.c_str());
  TEST_CHECK_(relative == "tex/grass.png",
      "An empty Texture must take RelativeFilename from its Video, got '%s'",
      relative.c_str());
}

void test_fbx_video_lowercase_file_name_element() {
  const Si64 kTextureId = 100;
  const Si64 kVideoId = 200;

  FbxNode objects("Objects");
  objects.Child(FbxTextureNode(kTextureId, nullptr, nullptr));
  objects.Child(FbxVideoNode(kVideoId, "Filename", "C:/art/brick.png",
      nullptr));

  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kVideoId, kTextureId));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene with a lowercase Filename must load, got error '%s'",
      ofbx::getError())) {
    return;
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::TEXTURE);
  if (!TEST_CHECK_(object != nullptr, "The Texture must be in the scene")) {
    return;
  }

  const ofbx::Texture *texture = static_cast<const ofbx::Texture*>(object);
  std::string filename = FbxToString(texture->getFileName());
  TEST_CHECK_(filename == "C:/art/brick.png",
      "'Filename' must be read like 'FileName', got '%s'", filename.c_str());
}

void test_fbx_video_does_not_override_texture_file_name() {
  const Si64 kTextureId = 100;
  const Si64 kVideoId = 200;

  FbxNode objects("Objects");
  objects.Child(FbxTextureNode(kTextureId, "own.png", "tex/own.png"));
  objects.Child(FbxVideoNode(kVideoId, "FileName", "clip.png", "tex/clip.png"));

  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kVideoId, kTextureId));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene with a Video clip must load, got error '%s'",
      ofbx::getError())) {
    return;
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::TEXTURE);
  if (!TEST_CHECK_(object != nullptr, "The Texture must be in the scene")) {
    return;
  }

  const ofbx::Texture *texture = static_cast<const ofbx::Texture*>(object);
  std::string filename = FbxToString(texture->getFileName());
  std::string relative = FbxToString(texture->getRelativeFileName());
  TEST_CHECK_(filename == "own.png",
      "A Texture with a FileName of its own must keep it, got '%s'",
      filename.c_str());
  TEST_CHECK_(relative == "tex/own.png",
      "A Texture with a RelativeFilename of its own must keep it, got '%s'",
      relative.c_str());
}

void test_fbx_layered_texture_reaches_material() {
  const Si64 kMaterialId = 100;
  const Si64 kLayeredId = 200;
  const Si64 kTextureId = 300;

  FbxNode objects("Objects");
  objects.Child(FbxMaterialNode(kMaterialId));
  objects.Child(FbxLayeredTextureNode(kLayeredId));
  objects.Child(FbxTextureNode(kTextureId, "layered.png", "tex/layered.png"));

  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kTextureId, kLayeredId));
  connections.Child(FbxConnectOP(kLayeredId, kMaterialId, "DiffuseColor"));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene with a LayeredTexture must load, got error '%s'",
      ofbx::getError())) {
    return;
  }

  const ofbx::Object *material_object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::MATERIAL);
  if (!TEST_CHECK_(material_object != nullptr,
      "The Material must be in the scene")) {
    return;
  }

  const ofbx::Material *material =
      static_cast<const ofbx::Material*>(material_object);
  const ofbx::Texture *texture =
      material->getTexture(ofbx::Texture::DIFFUSE);
  if (!TEST_CHECK_(texture != nullptr,
      "A Texture behind a LayeredTexture must reach the material")) {
    return;
  }
  std::string filename = FbxToString(texture->getFileName());
  TEST_CHECK_(filename == "layered.png",
      "The material must resolve to 'layered.png', got '%s'",
      filename.c_str());

  const ofbx::Object *layered_object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::LAYERED_TEXTURE);
  if (!TEST_CHECK_(layered_object != nullptr,
      "The LayeredTexture must be in the scene")) {
    return;
  }
  const ofbx::LayeredTexture *layered =
      static_cast<const ofbx::LayeredTexture*>(layered_object);
  TEST_CHECK_(layered->getTexture() == texture,
      "The LayeredTexture must expose the same Texture the material got");
}

void test_fbx_layered_texture_keeps_the_bottom_layer() {
  const Si64 kMaterialId = 100;
  const Si64 kLayeredId = 200;
  const Si64 kBottomId = 300;
  const Si64 kTopId = 400;

  FbxNode objects("Objects");
  objects.Child(FbxMaterialNode(kMaterialId));
  objects.Child(FbxLayeredTextureNode(kLayeredId));
  objects.Child(FbxTextureNode(kBottomId, "bottom.png", "tex/bottom.png"));
  objects.Child(FbxTextureNode(kTopId, "top.png", "tex/top.png"));

  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kBottomId, kLayeredId));
  connections.Child(FbxConnectOO(kTopId, kLayeredId));
  connections.Child(FbxConnectOP(kLayeredId, kMaterialId, "DiffuseColor"));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene with two layers must load, got error '%s'", ofbx::getError())) {
    return;
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::MATERIAL);
  if (!TEST_CHECK_(object != nullptr, "The Material must be in the scene")) {
    return;
  }

  const ofbx::Material *material =
      static_cast<const ofbx::Material*>(object);
  const ofbx::Texture *texture =
      material->getTexture(ofbx::Texture::DIFFUSE);
  if (!TEST_CHECK_(texture != nullptr,
      "A LayeredTexture with two layers must still reach the material")) {
    return;
  }
  std::string filename = FbxToString(texture->getFileName());
  TEST_CHECK_(filename == "bottom.png",
      "The layer connected first must win, got '%s'", filename.c_str());
}

void test_fbx_video_behind_layered_texture() {
  const Si64 kMaterialId = 100;
  const Si64 kLayeredId = 200;
  const Si64 kTextureId = 300;
  const Si64 kVideoId = 400;

  FbxNode objects("Objects");
  objects.Child(FbxMaterialNode(kMaterialId));
  objects.Child(FbxLayeredTextureNode(kLayeredId));
  objects.Child(FbxTextureNode(kTextureId, nullptr, nullptr));
  objects.Child(FbxVideoNode(kVideoId, "FileName", "grass.png",
      "tex/grass.png"));

  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kVideoId, kTextureId));
  connections.Child(FbxConnectOO(kTextureId, kLayeredId));
  connections.Child(FbxConnectOP(kLayeredId, kMaterialId, "Maya|DiffuseColor"));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "Video, LayeredTexture and Material together must load, got error '%s'",
      ofbx::getError())) {
    return;
  }

  const ofbx::Object *object = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::MATERIAL);
  if (!TEST_CHECK_(object != nullptr, "The Material must be in the scene")) {
    return;
  }

  const ofbx::Material *material =
      static_cast<const ofbx::Material*>(object);
  const ofbx::Texture *texture =
      material->getTexture(ofbx::Texture::DIFFUSE);
  if (!TEST_CHECK_(texture != nullptr,
      "The material must resolve a texture through the whole chain")) {
    return;
  }
  std::string filename = FbxToString(texture->getFileName());
  TEST_CHECK_(filename == "grass.png",
      "The path must come from the Video at the end of the chain, got '%s'",
      filename.c_str());
}

void test_fbx_tolerates_unexpected_connection() {
  const Si64 kModelId = 100;
  const Si64 kGeometryId = 200;
  const Si64 kTextureId = 300;

  const double kTriangle[9] = {
    0.0, 0.0, 0.0,
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0
  };
  const Si32 kPolygon[3] = {0, 1, -3};

  FbxNode geometry("Geometry");
  geometry.Long(kGeometryId).Str("Geometry::tri").Str("Mesh");
  geometry.Child(FbxNode("Vertices").DoubleArray(
      std::vector<double>(kTriangle, kTriangle + 9)));
  geometry.Child(FbxNode("PolygonVertexIndex").IntArray(
      std::vector<Si32>(kPolygon, kPolygon + 3)));

  FbxNode model("Model");
  model.Long(kModelId).Str("Model::tri").Str("Mesh");

  FbxNode objects("Objects");
  objects.Child(geometry);
  objects.Child(model);
  objects.Child(FbxTextureNode(kTextureId, "stray.png", nullptr));

  // A Texture hanging directly off a Model is not a link the loader knows how
  // to use, and it must be skipped rather than abort the whole scene.
  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeometryId, kModelId));
  connections.Child(FbxConnectOO(kTextureId, kModelId));

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "An unusable connection must not fail the load, got error '%s'",
      ofbx::getError())) {
    return;
  }
  if (!TEST_CHECK_(scene.Get()->getMeshCount() == 1,
      "The mesh must survive the stray connection, got %d meshes",
      scene.Get()->getMeshCount())) {
    return;
  }
  const ofbx::Geometry *geom = scene.Get()->getMesh(0)->getGeometry();
  TEST_CHECK_(geom != nullptr && geom->getVertexCount() == 3,
      "The geometry must still be attached and hold 3 vertices");
}

void test_fbx_object_types_and_count() {
  const Si64 kMaterialId = 100;
  const Si64 kLayeredId = 200;
  const Si64 kTextureId = 300;
  const Si64 kVideoId = 400;

  FbxNode objects("Objects");
  objects.Child(FbxMaterialNode(kMaterialId));
  objects.Child(FbxLayeredTextureNode(kLayeredId));
  objects.Child(FbxTextureNode(kTextureId, "tex.png", nullptr));
  objects.Child(FbxVideoNode(kVideoId, "FileName", "clip.png", nullptr));
  FbxNode connections("Connections");

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene of four objects must load, got error '%s'", ofbx::getError())) {
    return;
  }
  TEST_CHECK_(scene.Get()->getAllObjectCount() == 4,
      "Expected 4 objects, got %d", scene.Get()->getAllObjectCount());

  TEST_CHECK_(FbxFindObject(*scene.Get(),
      ofbx::Object::Type::MATERIAL) != nullptr, "Material is missing");
  TEST_CHECK_(FbxFindObject(*scene.Get(),
      ofbx::Object::Type::TEXTURE) != nullptr, "Texture is missing");
  TEST_CHECK_(FbxFindObject(*scene.Get(),
      ofbx::Object::Type::VIDEO) != nullptr, "Video is missing");
  TEST_CHECK_(FbxFindObject(*scene.Get(),
      ofbx::Object::Type::LAYERED_TEXTURE) != nullptr,
      "LayeredTexture is missing");

  const ofbx::Object *video = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::VIDEO);
  if (!TEST_CHECK_(video != nullptr, "Video is missing")) {
    return;
  }
  std::string filename =
      FbxToString(static_cast<const ofbx::Video*>(video)->getFileName());
  TEST_CHECK_(filename == "clip.png",
      "A standalone Video must keep its own FileName, got '%s'",
      filename.c_str());
  TEST_CHECK_(!video->isNode(),
      "A Video is not a scene node and must not be reported as one");

  const ofbx::Object *layered = FbxFindObject(*scene.Get(),
      ofbx::Object::Type::LAYERED_TEXTURE);
  if (!TEST_CHECK_(layered != nullptr, "LayeredTexture is missing")) {
    return;
  }
  TEST_CHECK_(static_cast<const ofbx::LayeredTexture*>(layered)
      ->getTexture() == nullptr,
      "A LayeredTexture with no layer connected must expose no texture");
  TEST_CHECK_(!layered->isNode(),
      "A LayeredTexture is not a scene node and must not be reported as one");
}

// The file numbers its axes from zero (0 = X, 1 = Y, 2 = Z) while UpVector
// numbers them from one. Handing the raw number straight to the enum shifts
// every answer by one axis, which silently turns a Z-up scene into a Y-up one
// and makes UpVector_AxisZ unreachable for any real exporter.
void test_fbx_global_settings_up_axis() {
  struct AxisCase {
    Si32 raw;
    ofbx::UpVector expected;
    const char *axis;
  };
  const AxisCase cases[] = {
    {0, ofbx::UpVector_AxisX, "X"},
    {1, ofbx::UpVector_AxisY, "Y"},
    {2, ofbx::UpVector_AxisZ, "Z"}
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    FbxNode properties("Properties70");
    properties.Child(FbxIntProperty("UpAxis", cases[i].raw));
    FbxNode objects("Objects");
    FbxNode connections("Connections");

    FbxScene scene(FbxBuildSceneWithSettings(
        FbxGlobalSettingsNode(properties), objects, connections));
    if (!TEST_CHECK_(scene.Get() != nullptr,
        "A scene carrying GlobalSettings must load, got error '%s'",
        ofbx::getError())) {
      continue;
    }
    const ofbx::GlobalSettings *settings = scene.Get()->getGlobalSettings();
    if (!TEST_CHECK_(settings != nullptr,
        "GlobalSettings must be reported for UpAxis %d", cases[i].raw)) {
      continue;
    }
    TEST_CHECK_(settings->UpAxis == cases[i].expected,
        "UpAxis %d in the file means %s up, so the enum must read %d, got %d",
        cases[i].raw, cases[i].axis, static_cast<int>(cases[i].expected),
        static_cast<int>(settings->UpAxis));
  }
}

// Only the axis enums are renumbered. The signs, the raw original axis and
// the unit scale are passed through as written, so shifting them too would be
// just as wrong as not shifting UpAxis.
void test_fbx_global_settings_pass_other_fields_through() {
  FbxNode properties("Properties70");
  properties.Child(FbxIntProperty("UpAxis", 2));
  properties.Child(FbxIntProperty("UpAxisSign", -1));
  properties.Child(FbxIntProperty("OriginalUpAxis", 2));
  properties.Child(FbxDoubleProperty("UnitScaleFactor", 2.54));
  FbxNode objects("Objects");
  FbxNode connections("Connections");

  FbxScene scene(FbxBuildSceneWithSettings(
      FbxGlobalSettingsNode(properties), objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene carrying GlobalSettings must load, got error '%s'",
      ofbx::getError())) {
    return;
  }
  const ofbx::GlobalSettings *settings = scene.Get()->getGlobalSettings();
  if (!TEST_CHECK_(settings != nullptr, "GlobalSettings must be reported")) {
    return;
  }
  TEST_CHECK_(settings->UpAxis == ofbx::UpVector_AxisZ,
      "A file saying UpAxis 2 is Z-up, got enum %d",
      static_cast<int>(settings->UpAxis));
  TEST_CHECK_(settings->UpAxisSign == -1,
      "UpAxisSign is carried through unchanged, got %d",
      settings->UpAxisSign);
  TEST_CHECK_(settings->OriginalUpAxis == 2,
      "OriginalUpAxis stays the raw file value, got %d",
      settings->OriginalUpAxis);
  TEST_CHECK_(std::fabs(settings->UnitScaleFactor - 2.54f) < 1e-4f,
      "UnitScaleFactor is carried through unchanged, got %f",
      settings->UnitScaleFactor);
}

// A file without a GlobalSettings block must not be treated as a broken one.
void test_fbx_missing_global_settings_is_not_an_error() {
  FbxNode objects("Objects");
  FbxNode connections("Connections");

  FbxScene scene(FbxBuildScene(objects, connections));
  if (!TEST_CHECK_(scene.Get() != nullptr,
      "A scene without GlobalSettings must load, got error '%s'",
      ofbx::getError())) {
    return;
  }
  TEST_CHECK_(scene.Get()->getGlobalSettings() != nullptr,
      "GlobalSettings must stay readable even when the file omits them");
}

std::vector<double> FbxIdentity16() {
  double m[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  };
  return std::vector<double>(m, m + 16);
}

FbxNode FbxTriangleGeometry(Si64 id) {
  const double kTri[9] = {
    0.0, 0.0, 0.0,
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0
  };
  const Si32 kPoly[3] = {0, 1, -3};
  FbxNode geometry("Geometry");
  geometry.Long(id).Str("Geometry::tri").Str("Mesh");
  geometry.Child(FbxNode("Vertices").DoubleArray(
      std::vector<double>(kTri, kTri + 9)));
  geometry.Child(FbxNode("PolygonVertexIndex").IntArray(
      std::vector<Si32>(kPoly, kPoly + 3)));
  return geometry;
}

FbxNode FbxQuadGeometry(Si64 id) {
  const double kQuad[12] = {
    0.0, 0.0, 0.0,
    1.0, 0.0, 0.0,
    1.0, 1.0, 0.0,
    0.0, 1.0, 0.0
  };
  const Si32 kPolygon[4] = {0, 1, 2, -4};
  FbxNode geometry("Geometry");
  geometry.Long(id).Str("Geometry::quad").Str("Mesh");
  geometry.Child(FbxNode("Vertices").DoubleArray(
      std::vector<double>(kQuad, kQuad + 12)));
  geometry.Child(FbxNode("PolygonVertexIndex").IntArray(
      std::vector<Si32>(kPolygon, kPolygon + 4)));
  return geometry;
}

void test_loadfbx_empty_buffer_fails() {
  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  TEST_CHECK_(!LoadFbx(&model, nullptr, 0, opt),
      "An empty buffer must not produce a model");
  TEST_CHECK_(model.parts.empty(), "A failed load must leave parts empty");
}

void test_loadfbx_scene_without_meshes_fails() {
  FbxNode objects("Objects");
  FbxNode connections("Connections");
  std::vector<Ui8> data = FbxBuildScene(objects, connections);
  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  TEST_CHECK_(!LoadFbx(&model, data.data(), static_cast<Si32>(data.size()),
      opt), "A scene with no mesh must not count as a loaded model");
  TEST_CHECK_(model.parts.empty(), "No mesh means no parts");
  TEST_CHECK_(model.bones.empty() && model.clips.empty(),
      "A file without Skin must leave bones and clips empty");
}

void test_loadfbx_quad_is_two_triangles() {
  const Si64 kModelId = 100;
  const Si64 kGeometryId = 200;
  FbxNode objects("Objects");
  objects.Child(FbxQuadGeometry(kGeometryId));
  FbxNode model_node("Model");
  model_node.Long(kModelId).Str("Model::quad").Str("Mesh");
  objects.Child(model_node);
  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeometryId, kModelId));
  std::vector<Ui8> data = FbxBuildScene(objects, connections);

  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  if (!TEST_CHECK_(LoadFbx(&model, data.data(),
      static_cast<Si32>(data.size()), opt),
      "A quad mesh must load")) {
    return;
  }
  if (!TEST_CHECK_(model.parts.size() == 1,
      "One materialless mesh is one part, got %d",
      static_cast<int>(model.parts.size()))) {
    return;
  }
  Mesh *mesh = model.parts[0].mesh.get();
  if (!TEST_CHECK_(mesh != nullptr, "The part must own a mesh")) {
    return;
  }
  TEST_CHECK_(mesh->GetCurrentVertexCount(0) == 6,
      "A quad is two triangles, 6 vertices, got %d",
      mesh->GetCurrentVertexCount(0));
  TEST_CHECK_(mesh->GetCurrentFaceCount(0) == 2,
      "A quad is two faces, got %d", mesh->GetCurrentFaceCount(0));
  TEST_CHECK_(model.bones.empty() && model.parts[0].skin.size() == 6,
      "A rigid mesh still has a skin list per vertex");
  bool all_empty = true;
  for (size_t i = 0; i < model.parts[0].skin.size(); ++i) {
    if (!model.parts[0].skin[i].empty()) {
      all_empty = false;
    }
  }
  TEST_CHECK_(all_empty, "A file without Skin must have empty influence lists");
  TEST_CHECK_(mesh->mBBox.min_x > -0.01f && mesh->mBBox.max_x < 1.01f,
      "BBox x must cover the unit quad, got %f..%f",
      mesh->mBBox.min_x, mesh->mBBox.max_x);
  TEST_CHECK_(mesh->mBBox.min_y > -0.01f && mesh->mBBox.max_y < 1.01f,
      "BBox y must cover the unit quad, got %f..%f",
      mesh->mBBox.min_y, mesh->mBBox.max_y);
  float *n0 = static_cast<float*>(mesh->GetVertexData(0, 0, 1));
  TEST_CHECK_(n0 != nullptr && n0[2] > 0.9f,
      "A quad in z=0 must get +Z normals, got %f", n0 ? n0[2] : 0.0f);
}

void test_loadfbx_z_up_swaps_y_and_z() {
  const Si64 kModelId = 100;
  const Si64 kGeometryId = 200;
  FbxNode properties("Properties70");
  properties.Child(FbxIntProperty("UpAxis", 2));
  FbxNode objects("Objects");
  objects.Child(FbxQuadGeometry(kGeometryId));
  FbxNode model_node("Model");
  model_node.Long(kModelId).Str("Model::quad").Str("Mesh");
  objects.Child(model_node);
  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeometryId, kModelId));
  std::vector<Ui8> data = FbxBuildSceneWithSettings(
      FbxGlobalSettingsNode(properties), objects, connections);

  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  if (!TEST_CHECK_(LoadFbx(&model, data.data(),
      static_cast<Si32>(data.size()), opt),
      "A Z-up quad must load")) {
    return;
  }
  TEST_CHECK_(model.z_up, "UpAxis Z must be recorded on the model");
  Mesh *mesh = model.parts[0].mesh.get();
  if (!TEST_CHECK_(mesh != nullptr, "The part must own a mesh")) {
    return;
  }
  TEST_CHECK_(mesh->mBBox.max_y - mesh->mBBox.min_y < 0.01f,
      "After Z-up the quad lies in y=0, y span was %f",
      mesh->mBBox.max_y - mesh->mBBox.min_y);
  TEST_CHECK_(mesh->mBBox.min_z < -0.9f && mesh->mBBox.max_z < 0.01f,
      "After Z-up former +Y becomes -Z, bbox z %f..%f",
      mesh->mBBox.min_z, mesh->mBBox.max_z);
}

void test_loadfbx_two_meshes_are_two_parts() {
  const Si64 kModelA = 100;
  const Si64 kGeomA = 200;
  const Si64 kMatA = 300;
  const Si64 kModelB = 400;
  const Si64 kGeomB = 500;
  const Si64 kMatB = 600;
  FbxNode objects("Objects");
  objects.Child(FbxTriangleGeometry(kGeomA));
  objects.Child(FbxTriangleGeometry(kGeomB));
  FbxNode ma("Model");
  ma.Long(kModelA).Str("Model::a").Str("Mesh");
  FbxNode mb("Model");
  mb.Long(kModelB).Str("Model::b").Str("Mesh");
  objects.Child(ma);
  objects.Child(mb);
  FbxNode mata("Material");
  mata.Long(kMatA).Str("Material::red").Str("");
  FbxNode matb("Material");
  matb.Long(kMatB).Str("Material::blue").Str("");
  objects.Child(mata);
  objects.Child(matb);
  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeomA, kModelA));
  connections.Child(FbxConnectOO(kGeomB, kModelB));
  connections.Child(FbxConnectOO(kMatA, kModelA));
  connections.Child(FbxConnectOO(kMatB, kModelB));
  std::vector<Ui8> data = FbxBuildScene(objects, connections);

  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  if (!TEST_CHECK_(LoadFbx(&model, data.data(),
      static_cast<Si32>(data.size()), opt),
      "Two meshed materials must load")) {
    return;
  }
  TEST_CHECK_(model.parts.size() == 2,
      "Different materials must not merge, got %d parts",
      static_cast<int>(model.parts.size()));
}

void test_resolve_asset_path_finds_basename_and_stem() {
  Sprite spr;
  spr.Create(4, 4);
  spr.Clear(Rgba(10, 20, 30, 255));
  const char *name = "FbBridgeTex_Case.png";
  spr.Save(name);
  if (!TEST_CHECK_(DoesFileExist(name) == kTrivalentTrue,
      "The fixture PNG must have been written")) {
    return;
  }
  std::vector<std::string> dirs;
  dirs.push_back(".");
  std::string hit = ResolveAssetPath("missing/FbBridgeTex_Case.jpg", dirs);
  TEST_CHECK_(hit.find("FbBridgeTex_Case.png") != std::string::npos,
      "A jpg name must resolve to the png in the search dir, got '%s'",
      hit.c_str());
  std::string case_hit = ResolveAssetPath("fbbridgetex_case.TGA", dirs);
  TEST_CHECK_(case_hit.find("FbBridgeTex_Case.png") != std::string::npos,
      "A case-insensitive stem must resolve, got '%s'", case_hit.c_str());
  std::string missing = ResolveAssetPath("no_such_texture_zzq.png", dirs);
  TEST_CHECK_(missing.empty(),
      "A name that is not on disk must come back empty, got '%s'",
      missing.c_str());
  std::remove(name);
}

void test_loadfbx_skin_and_cluster() {
  const Si64 kModelId = 100;
  const Si64 kGeomId = 200;
  const Si64 kSkinId = 300;
  const Si64 kClusterId = 400;
  const Si64 kLimbId = 500;
  FbxNode objects("Objects");
  objects.Child(FbxTriangleGeometry(kGeomId));
  FbxNode mesh("Model");
  mesh.Long(kModelId).Str("Model::tri").Str("Mesh");
  objects.Child(mesh);
  FbxNode limb("Model");
  limb.Long(kLimbId).Str("Model::joint").Str("LimbNode");
  objects.Child(limb);
  FbxNode skin("Deformer");
  skin.Long(kSkinId).Str("Deformer::Skin").Str("Skin");
  objects.Child(skin);
  FbxNode cluster("Deformer");
  cluster.Long(kClusterId).Str("Deformer::Cluster").Str("Cluster");
  const Si32 kIdx[3] = {0, 1, 2};
  const double kW[3] = {1.0, 0.5, 0.25};
  double link[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  };
  link[12] = 3.0;
  link[13] = 4.0;
  link[14] = 5.0;
  cluster.Child(FbxNode("Indexes").IntArray(
      std::vector<Si32>(kIdx, kIdx + 3)));
  cluster.Child(FbxNode("Weights").DoubleArray(
      std::vector<double>(kW, kW + 3)));
  cluster.Child(FbxNode("Transform").DoubleArray(FbxIdentity16()));
  cluster.Child(FbxNode("TransformLink").DoubleArray(
      std::vector<double>(link, link + 16)));
  objects.Child(cluster);
  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeomId, kModelId));
  connections.Child(FbxConnectOO(kSkinId, kGeomId));
  connections.Child(FbxConnectOO(kClusterId, kSkinId));
  connections.Child(FbxConnectOO(kLimbId, kClusterId));
  std::vector<Ui8> data = FbxBuildScene(objects, connections);

  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  if (!TEST_CHECK_(LoadFbx(&model, data.data(),
      static_cast<Si32>(data.size()), opt),
      "A skinned triangle must load, ofbx said '%s'", ofbx::getError())) {
    return;
  }
  if (!TEST_CHECK_(model.bones.size() == 1,
      "One LimbNode cluster link is one bone, got %d",
      static_cast<int>(model.bones.size()))) {
    return;
  }
  TEST_CHECK_(std::string(model.bones[0].name).find("joint") !=
      std::string::npos,
      "The bone must keep the LimbNode name, got '%s'",
      model.bones[0].name.c_str());
  TEST_CHECK_(std::fabs(model.bones[0].inverse_bind.m[3] - 3.0f) < 1e-4f
      && std::fabs(model.bones[0].inverse_bind.m[7] - 4.0f) < 1e-4f
      && std::fabs(model.bones[0].inverse_bind.m[11] - 5.0f) < 1e-4f,
      "Inverse bind translation must be (3,4,5), got (%f,%f,%f)",
      model.bones[0].inverse_bind.m[3],
      model.bones[0].inverse_bind.m[7],
      model.bones[0].inverse_bind.m[11]);
  if (!TEST_CHECK_(model.parts.size() == 1 &&
      model.parts[0].skin.size() == 3,
      "The triangle must keep 3 skinned vertices")) {
    return;
  }
  const float expect[3] = {1.0f, 0.5f, 0.25f};
  for (int i = 0; i < 3; ++i) {
    if (!TEST_CHECK_(model.parts[0].skin[static_cast<size_t>(i)].size() == 1,
        "Vertex %d must have one influence, got %d",
        i, static_cast<int>(model.parts[0].skin[static_cast<size_t>(i)].size()))) {
      return;
    }
    TEST_CHECK_(model.parts[0].skin[static_cast<size_t>(i)][0].bone == 0,
        "Vertex %d must point at bone 0", i);
    TEST_CHECK_(std::fabs(model.parts[0].skin[static_cast<size_t>(i)][0].weight
        - expect[i]) < 1e-5f,
        "Vertex %d weight must be %f, got %f",
        i, expect[i],
        model.parts[0].skin[static_cast<size_t>(i)][0].weight);
  }
}

void test_loadfbx_animation_curve_keys() {
  const Si64 kModelId = 100;
  const Si64 kGeomId = 200;
  const Si64 kSkinId = 300;
  const Si64 kClusterId = 400;
  const Si64 kLimbId = 500;
  const Si64 kStackId = 600;
  const Si64 kLayerId = 700;
  const Si64 kNodeId = 800;
  const Si64 kCurveId = 900;
  FbxNode objects("Objects");
  objects.Child(FbxTriangleGeometry(kGeomId));
  FbxNode mesh("Model");
  mesh.Long(kModelId).Str("Model::tri").Str("Mesh");
  objects.Child(mesh);
  FbxNode limb("Model");
  limb.Long(kLimbId).Str("Model::joint").Str("LimbNode");
  objects.Child(limb);
  FbxNode skin("Deformer");
  skin.Long(kSkinId).Str("Deformer::Skin").Str("Skin");
  objects.Child(skin);
  FbxNode cluster("Deformer");
  cluster.Long(kClusterId).Str("Deformer::Cluster").Str("Cluster");
  const Si32 kIdx[3] = {0, 1, 2};
  const double kW[3] = {1.0, 1.0, 1.0};
  cluster.Child(FbxNode("Indexes").IntArray(
      std::vector<Si32>(kIdx, kIdx + 3)));
  cluster.Child(FbxNode("Weights").DoubleArray(
      std::vector<double>(kW, kW + 3)));
  cluster.Child(FbxNode("Transform").DoubleArray(FbxIdentity16()));
  cluster.Child(FbxNode("TransformLink").DoubleArray(FbxIdentity16()));
  objects.Child(cluster);
  FbxNode stack("AnimationStack");
  stack.Long(kStackId).Str("AnimStack::clip").Str("");
  objects.Child(stack);
  FbxNode layer("AnimationLayer");
  layer.Long(kLayerId).Str("AnimLayer::base").Str("");
  objects.Child(layer);
  FbxNode cnode("AnimationCurveNode");
  cnode.Long(kNodeId).Str("AnimCurveNode::T").Str("");
  objects.Child(cnode);
  FbxNode curve("AnimationCurve");
  curve.Long(kCurveId).Str("AnimCurve::x").Str("");
  const Si64 kTimes[2] = {0, 46186158000LL};
  const float kVals[2] = {0.0f, 2.5f};
  curve.Child(FbxNode("KeyTime").LongArray(
      std::vector<Si64>(kTimes, kTimes + 2)));
  curve.Child(FbxNode("KeyValueFloat").FloatArray(
      std::vector<float>(kVals, kVals + 2)));
  objects.Child(curve);
  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeomId, kModelId));
  connections.Child(FbxConnectOO(kSkinId, kGeomId));
  connections.Child(FbxConnectOO(kClusterId, kSkinId));
  connections.Child(FbxConnectOO(kLimbId, kClusterId));
  connections.Child(FbxConnectOO(kLayerId, kStackId));
  connections.Child(FbxConnectOO(kNodeId, kLayerId));
  connections.Child(FbxConnectOO(kCurveId, kNodeId));
  connections.Child(FbxConnectOP(kNodeId, kLimbId, "Lcl Translation"));
  std::vector<Ui8> data = FbxBuildScene(objects, connections);

  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  if (!TEST_CHECK_(LoadFbx(&model, data.data(),
      static_cast<Si32>(data.size()), opt),
      "A skinned triangle with a curve must load, ofbx said '%s'",
      ofbx::getError())) {
    return;
  }
  if (!TEST_CHECK_(!model.clips.empty() && !model.clips[0].curves.empty(),
      "The clip must keep the raw curve, clips=%d",
      static_cast<int>(model.clips.size()))) {
    return;
  }
  const AnimCurve &ac = model.clips[0].curves[0];
  TEST_CHECK_(ac.property.find("Lcl Translation") != std::string::npos,
      "The curve property must be Lcl Translation, got '%s'",
      ac.property.c_str());
  TEST_CHECK_(ac.bone == 0, "The curve must point at bone 0, got %d", ac.bone);
  TEST_CHECK_(ac.axis == 0, "The first connected curve is axis 0, got %d",
      ac.axis);
  if (!TEST_CHECK_(ac.times.size() == 2 && ac.values.size() == 2,
      "Two keys must be copied, got %d times",
      static_cast<int>(ac.times.size()))) {
    return;
  }
  TEST_CHECK_(ac.times[0] == 0 && ac.times[1] == 46186158000LL,
      "Key times must be copied unchanged");
  TEST_CHECK_(std::fabs(ac.values[0] - 0.0f) < 1e-5f
      && std::fabs(ac.values[1] - 2.5f) < 1e-5f,
      "Key values must be copied unchanged, got %f %f",
      ac.values[0], ac.values[1]);
}

void test_loadfbx_texture_path_unresolved() {
  const Si64 kModelId = 100;
  const Si64 kGeomId = 200;
  const Si64 kMatId = 300;
  const Si64 kTexId = 400;
  FbxNode objects("Objects");
  objects.Child(FbxTriangleGeometry(kGeomId));
  FbxNode mesh("Model");
  mesh.Long(kModelId).Str("Model::tri").Str("Mesh");
  objects.Child(mesh);
  objects.Child(FbxMaterialNode(kMatId));
  objects.Child(FbxTextureNode(kTexId, "missing_bridge.png",
      "tex/missing_bridge.png"));
  FbxNode connections("Connections");
  connections.Child(FbxConnectOO(kGeomId, kModelId));
  connections.Child(FbxConnectOO(kMatId, kModelId));
  connections.Child(FbxConnectOP(kTexId, kMatId, "DiffuseColor"));
  std::vector<Ui8> data = FbxBuildScene(objects, connections);

  Model model;
  FbxLoadOptions opt;
  opt.upload_textures = false;
  opt.search_dirs.push_back(".");
  if (!TEST_CHECK_(LoadFbx(&model, data.data(),
      static_cast<Si32>(data.size()), opt),
      "A textured triangle must load")) {
    return;
  }
  if (!TEST_CHECK_(!model.parts.empty(), "There must be a part")) {
    return;
  }
  TEST_CHECK_(!model.parts[0].raw_tex.empty(),
      "The raw texture path from the file must be kept");
  TEST_CHECK_(model.parts[0].diffuse.get() == nullptr,
      "upload_textures=false must leave the GPU pointer empty");
  TEST_CHECK_(model.parts[0].resolved_tex.empty(),
      "Without a file on disk resolved_tex must be empty, got '%s'",
      model.parts[0].resolved_tex.c_str());
  TEST_CHECK_(model.parts[0].raw_tex.find("missing_bridge") !=
      std::string::npos,
      "The raw name from the file must mention the missing texture");
}
