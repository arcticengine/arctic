// Copyright (c) <year> Your name

#include "engine/easy.h"
#include "engine/mat44f.h"
#include "engine/vec3f.h"
#include "engine/gl_framebuffer.h"

using namespace arctic;  // NOLINT

HwSprite g_render_target;
Mesh g_mesh;
GlBuffer g_vbo;
GlBuffer g_ebo;
GlProgram g_program;
GlTexture2D g_texture;

Vec3F g_camera_pos(0.0f, 1.5f, 4.0f);
Vec3F g_camera_up(0.0f, 1.0f, 0.0f);
float g_yaw = 3.14159f;
float g_pitch = -0.3f;
float g_camera_speed = 0.05f;
float g_mouse_sensitivity = 0.002f;

static MeshVertexFormat g_vertex_format;

static void AddQuad(Vec3F center, float half,
    Vec3F p0, Vec3F p1, Vec3F p2, Vec3F p3,
    Vec3F normal, float u0, float v0, float u1, float v1) {
  int base = g_mesh.GetCurrentVertexCount(0);
  g_mesh.AddVertex(0,
    center.x + p0.x * half, center.y + p0.y * half, center.z + p0.z * half,
    normal.x, normal.y, normal.z, u0, v0);
  g_mesh.AddVertex(0,
    center.x + p1.x * half, center.y + p1.y * half, center.z + p1.z * half,
    normal.x, normal.y, normal.z, u1, v0);
  g_mesh.AddVertex(0,
    center.x + p2.x * half, center.y + p2.y * half, center.z + p2.z * half,
    normal.x, normal.y, normal.z, u1, v1);
  g_mesh.AddVertex(0,
    center.x + p3.x * half, center.y + p3.y * half, center.z + p3.z * half,
    normal.x, normal.y, normal.z, u0, v1);
  g_mesh.AddFace(0, base, base + 1, base + 2);
  g_mesh.AddFace(0, base, base + 2, base + 3);
}

static void BuildCube() {
  g_vertex_format.AddElement(3, kRMVEDT_Float, false);  // position
  g_vertex_format.AddElement(3, kRMVEDT_Float, false);  // normal
  g_vertex_format.AddElement(2, kRMVEDT_Float, false);  // texcoord

  if (!g_mesh.Init(1, 24, &g_vertex_format, kRMVEDT_Polys, 1, 12)) {
    *Log() << "Failed to initialize mesh";
    return;
  }

  Vec3F center(0.0f, 0.0f, 0.0f);
  float h = 1.0f;

  AddQuad(center, h,
    Vec3F(-1, -1,  1), Vec3F( 1, -1,  1), Vec3F( 1,  1,  1), Vec3F(-1,  1,  1),
    Vec3F(0, 0, 1), 0, 0, 1, 1);
  AddQuad(center, h,
    Vec3F( 1, -1, -1), Vec3F(-1, -1, -1), Vec3F(-1,  1, -1), Vec3F( 1,  1, -1),
    Vec3F(0, 0, -1), 0, 0, 1, 1);
  AddQuad(center, h,
    Vec3F(-1,  1,  1), Vec3F( 1,  1,  1), Vec3F( 1,  1, -1), Vec3F(-1,  1, -1),
    Vec3F(0, 1, 0), 0, 0, 1, 1);
  AddQuad(center, h,
    Vec3F(-1, -1, -1), Vec3F( 1, -1, -1), Vec3F( 1, -1,  1), Vec3F(-1, -1,  1),
    Vec3F(0, -1, 0), 0, 0, 1, 1);
  AddQuad(center, h,
    Vec3F( 1, -1,  1), Vec3F( 1, -1, -1), Vec3F( 1,  1, -1), Vec3F( 1,  1,  1),
    Vec3F(1, 0, 0), 0, 0, 1, 1);
  AddQuad(center, h,
    Vec3F(-1, -1, -1), Vec3F(-1, -1,  1), Vec3F(-1,  1,  1), Vec3F(-1,  1, -1),
    Vec3F(-1, 0, 0), 0, 0, 1, 1);

  g_vbo.Create();
  g_ebo.Create();

  g_vbo.Bind(GL_ARRAY_BUFFER);
  g_vbo.SetData(g_mesh.mVertexData.mVertexArray[0].mBuffer,
    g_mesh.mVertexData.mVertexArray[0].mNum *
    g_mesh.mVertexData.mVertexArray[0].mFormat.mStride);

  g_ebo.Bind(GL_ELEMENT_ARRAY_BUFFER);
  g_ebo.SetData(g_mesh.mFaceData.mIndexArray[0].mBuffer,
    g_mesh.mFaceData.mIndexArray[0].mNum * sizeof(MeshFace));
}

static void CreateCheckerTexture() {
  const Si32 kSize = 64;
  const Si32 kCell = 8;
  Ui8 pixels[kSize * kSize * 4];
  for (Si32 y = 0; y < kSize; y++) {
    for (Si32 x = 0; x < kSize; x++) {
      bool white = ((x / kCell) + (y / kCell)) % 2 == 0;
      Si32 idx = (y * kSize + x) * 4;
      Ui8 c = white ? 255 : 60;
      pixels[idx + 0] = c;
      pixels[idx + 1] = c;
      pixels[idx + 2] = c;
      pixels[idx + 3] = 255;
    }
  }
  g_texture.Create(kSize, kSize);
  g_texture.Bind(0);
  g_texture.SetData(pixels, kSize, kSize);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

static void CreateShader() {
  const char vs[] = R"SHADER(
    #ifdef GL_ES
    precision highp float;
    #endif
    attribute vec3 vPosition;
    attribute vec3 vNormal;
    attribute vec2 vTexCoord;
    varying vec3 fNormal;
    varying vec3 fWorldPos;
    varying vec2 fTexCoord;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
      vec4 worldPos = model * vec4(vPosition, 1.0);
      fWorldPos = worldPos.xyz;
      fNormal = mat3(model[0].xyz, model[1].xyz, model[2].xyz) * vNormal;
      fTexCoord = vTexCoord;
      gl_Position = projection * view * worldPos;
    }
  )SHADER";

  const char fs[] = R"SHADER(
    #ifdef GL_ES
    precision highp float;
    #endif
    varying vec3 fNormal;
    varying vec3 fWorldPos;
    varying vec2 fTexCoord;
    uniform sampler2D texSampler;
    uniform vec3 lightDir;
    void main() {
      vec3 n = normalize(fNormal);
      float diff = max(dot(n, normalize(lightDir)), 0.0);
      float lighting = 0.25 + 0.75 * diff;
      vec4 texColor = texture2D(texSampler, fTexCoord);
      gl_FragColor = vec4(texColor.rgb * lighting, 1.0);
    }
  )SHADER";

  g_program.Create(vs, fs);
}

static void DrawMesh() {
  Si32 stride = g_mesh.mVertexData.mVertexArray[0].mFormat.mStride;
  g_vbo.Bind(GL_ARRAY_BUFFER);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
    (const GLvoid*)(uintptr_t)g_mesh.mVertexData.mVertexArray[0].mFormat.mElems[0].mOffset);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
    (const GLvoid*)(uintptr_t)g_mesh.mVertexData.mVertexArray[0].mFormat.mElems[1].mOffset);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
    (const GLvoid*)(uintptr_t)g_mesh.mVertexData.mVertexArray[0].mFormat.mElems[2].mOffset);
  glEnableVertexAttribArray(2);

  g_ebo.Bind(GL_ELEMENT_ARRAY_BUFFER);
  glDrawElements(GL_TRIANGLES,
    g_mesh.mFaceData.mIndexArray[0].mNum * 3, GL_UNSIGNED_INT, 0);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
}

void EasyMain() {
  Vec2Si32 screen_size = ScreenSize();
  g_render_target.Create(screen_size.x, screen_size.y);
  g_render_target.sprite_instance()->framebuffer().AttachDepthBuffer(
    screen_size.x, screen_size.y);

  BuildCube();
  CreateCheckerTexture();
  CreateShader();

  CaptureMouse();

  float cube_angle = 0.0f;

  while (!IsKeyDownward(kKeyEscape)) {
    Vec2Si32 mouse_delta = MouseMove();
    if (mouse_delta.x != 0 || mouse_delta.y != 0) {
      g_yaw -= mouse_delta.x * g_mouse_sensitivity;
      g_pitch += mouse_delta.y * g_mouse_sensitivity;
      float limit = static_cast<float>(kPi) * 0.49f;
      if (g_pitch > limit) {
        g_pitch = limit;
      }
      if (g_pitch < -limit) {
        g_pitch = -limit;
      }
    }

    Vec3F cam_dir = Normalize(Vec3F(
      sinf(g_yaw) * cosf(g_pitch),
      sinf(g_pitch),
      cosf(g_yaw) * cosf(g_pitch)));
    Vec3F right = Normalize(Cross(cam_dir, g_camera_up));

    if (IsKeyDown(kKeyW)) {
      g_camera_pos += cam_dir * g_camera_speed;
    }
    if (IsKeyDown(kKeyS)) {
      g_camera_pos -= cam_dir * g_camera_speed;
    }
    if (IsKeyDown(kKeyA)) {
      g_camera_pos -= right * g_camera_speed;
    }
    if (IsKeyDown(kKeyD)) {
      g_camera_pos += right * g_camera_speed;
    }

    cube_angle += 0.01f;

    g_render_target.sprite_instance()->framebuffer().Bind();
    screen_size = ScreenSize();
    glViewport(0, 0, screen_size.x, screen_size.y);
    glClearColor(0.15f, 0.15f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    Mat44F model = SetRotationY(cube_angle);
    Mat44F view = SetLookat(g_camera_pos, g_camera_pos + cam_dir, g_camera_up);
    float aspect = static_cast<float>(screen_size.x)
                 / static_cast<float>(screen_size.y);
    Mat44F projection = SetPerspective(45.0f, aspect, 0.1f, 100.0f);

    g_program.Bind();
    g_program.SetUniformTransposed("model", model);
    g_program.SetUniformTransposed("view", view);
    g_program.SetUniformTransposed("projection", projection);
    g_program.SetUniform("lightDir", Vec3F(0.5f, 1.0f, 0.3f));

    g_texture.Bind(0);
    g_program.SetUniform("texSampler", 0);

    DrawMesh();

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    GlFramebuffer::BindDefault();

    g_render_target.Draw(0, 0, kDrawBlendingModeAlphaBlend, kFilterNearest);
    ShowFrame();
    Clear();
  }

  ReleaseMouse();
}
