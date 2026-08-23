// Copyright (c) <year> Your name

#include <cstdio>

#include "engine/easy.h"
#include "engine/mat44f.h"
#include "engine/vec3f.h"
#include "engine/gl_framebuffer.h"

using namespace arctic;  // NOLINT

Font g_font;
HwSprite g_render_target;
Mesh g_mesh;
GlProgram g_program;
GlTexture2D g_texture;

Vec3F g_camera_pos(0.0f, 1.5f, 5.0f);
Vec3F g_camera_up(0.0f, 1.0f, 0.0f);
float g_yaw = 3.14159f;
float g_pitch = -0.3f;
float g_camera_speed = 3.0f;
float g_mouse_sensitivity = 0.002f;
float g_cube_rotation_speed = 0.6f;

// Two lamps circling the cube. They are separate uniforms rather than an array
// of two, because uniform arrays are a weak spot of some GLES and WebGL
// drivers, while plain vec3 uniforms work everywhere.
Rgba g_light_color_0(255, 240, 200);
Rgba g_light_color_1(120, 160, 255);

static MeshVertexFormat g_vertex_format;

// The size the render target was built for, so that a resized window is
// noticed and the target is built anew.
Vec2Si32 g_render_target_size(0, 0);

// =====================
// Culling and vertex order
// 
// With GL_CULL_FACE, GL_BACK, and GL_CCW, front vs back is decided only by
// vertex winding, not by your stored normals. Pick triangle order so that,
// when seen from where the camera is supposed to stand (the side you want
// visible), the vertices go counter‑clockwise. Wrong winding can drop the
// whole face even if normals look fine.
// =====================

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
  // Naming the elements after the attributes of the vertex shader is what lets
  // Mesh::Draw bind the vertex data by itself, with no glVertexAttribPointer
  // calls in sight. The names are not copied, so they have to be literals or
  // otherwise outlive the mesh.
  g_vertex_format.AddElement("vPosition", 3, kRMVEDT_Float, false);
  g_vertex_format.AddElement("vNormal", 3, kRMVEDT_Float, false);
  g_vertex_format.AddElement("vTexCoord", 2, kRMVEDT_Float, false);

  // A cube is 6 quads: 24 vertices and 12 triangles. AddVertex and AddFace
  // refuse to write past what is asked for here and say so in the log, so
  // count the geometry before allocating, or call Expand to make more room.
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
    uniform vec3 light0Pos;
    uniform vec3 light0Color;
    uniform vec3 light1Pos;
    uniform vec3 light1Color;
    vec3 PointLight(vec3 n, vec3 lightPos, vec3 lightColor) {
      vec3 toLight = lightPos - fWorldPos;
      float distance = length(toLight);
      float diff = max(dot(n, toLight / distance), 0.0);
      float attenuation = 3.0 / (1.0 + 0.5 * distance * distance);
      return lightColor * (diff * attenuation);
    }
    void main() {
      vec3 n = normalize(fNormal);
      vec3 lighting = vec3(0.2, 0.2, 0.24)
        + PointLight(n, light0Pos, light0Color)
        + PointLight(n, light1Pos, light1Color);
      vec4 texColor = texture2D(texSampler, fTexCoord);
      gl_FragColor = vec4(texColor.rgb * lighting, 1.0);
    }
  )SHADER";

  // Binding the attribute names before linking keeps the slots predictable.
  // Mesh::Draw asks the program where each named element goes anyway, so a
  // shader with attributes of other names would work just as well.
  g_program.Create(vs, fs, {"vPosition", "vNormal", "vTexCoord"});
}

// A 3d pass needs a color buffer of its own with a depth buffer attached, and
// it has to match the window, or the picture ends up drawn into a corner of it.
// ScreenSize is the resolution of the 2d backbuffer, WindowSize is the window
// in real pixels, and they part ways as soon as the window is resized.
static void ResizeRenderTarget(Vec2Si32 size) {
  if (size.x < 1 || size.y < 1) {
    return;
  }
  ResizeScreen(size);
  g_render_target.Create(size.x, size.y);
  g_render_target.sprite_instance()->framebuffer().AttachDepthBuffer(
    size.x, size.y);
  g_render_target_size = size;
}

void EasyMain() {
  ResizeRenderTarget(WindowSize());

  BuildCube();
  CreateCheckerTexture();
  CreateShader();

  g_font.LoadSystemFont("Arial", 20.0f);

  CaptureMouse();

  float cube_angle = 0.0f;
  double prev_time = Time();
  double fps_timer = prev_time;
  Si32 fps_frame_count = 0;
  Si32 fps_display = 0;

  while (!IsMainWindowCloseRequested() && !IsKeyDownward(kKeyEscape)) {
    double cur_time = Time();
    float dt = static_cast<float>(cur_time - prev_time);
    prev_time = cur_time;

    if (WindowSize() != g_render_target_size) {
      ResizeRenderTarget(WindowSize());
    }

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

    float move = g_camera_speed * dt;
    if (IsKeyDown(kKeyW)) {
      g_camera_pos += cam_dir * move;
    }
    if (IsKeyDown(kKeyS)) {
      g_camera_pos -= cam_dir * move;
    }
    if (IsKeyDown(kKeyA)) {
      g_camera_pos -= right * move;
    }
    if (IsKeyDown(kKeyD)) {
      g_camera_pos += right * move;
    }

    cube_angle += g_cube_rotation_speed * dt;

    Vec2Si32 screen_size = ScreenSize();
    g_render_target.sprite_instance()->framebuffer().Bind();
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

    // The lamps circle the cube in opposite directions, one above and one
    // below, so every face is lit in turn.
    float light_angle = static_cast<float>(cur_time);
    Vec3F light_pos_0(cosf(light_angle + 0.8f) * 2.6f, 1.6f,
      sinf(light_angle + 0.8f) * 2.6f);
    Vec3F light_pos_1(cosf(-light_angle * 0.7f) * 2.6f, -1.6f,
      sinf(-light_angle * 0.7f) * 2.6f);

    g_program.Bind();
    g_program.SetUniformTransposed("model", model);
    g_program.SetUniformTransposed("view", view);
    g_program.SetUniformTransposed("projection", projection);
    g_program.SetUniform("light0Pos", light_pos_0);
    g_program.SetUniform("light0Color", Vec3F(
      g_light_color_0.r / 255.0f,
      g_light_color_0.g / 255.0f,
      g_light_color_0.b / 255.0f));
    g_program.SetUniform("light1Pos", light_pos_1);
    g_program.SetUniform("light1Color", Vec3F(
      g_light_color_1.r / 255.0f,
      g_light_color_1.g / 255.0f,
      g_light_color_1.b / 255.0f));

    g_texture.Bind(0);
    g_program.SetUniform("texSampler", 0);

    // The mesh keeps its own vertex and index buffers, sends them to the GPU
    // when the geometry changes, and binds every named element to the
    // attribute of that name.
    g_mesh.Draw(g_program);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    GlFramebuffer::BindDefault();

    g_render_target.Draw(0, 0, kDrawBlendingModeAlphaBlend, kFilterNearest);

    fps_frame_count++;
    if (cur_time - fps_timer >= 1.0) {
      fps_display = fps_frame_count;
      fps_frame_count = 0;
      fps_timer = cur_time;
    }

    char fps_buf[32];
    snprintf(fps_buf, sizeof(fps_buf), "FPS: %d", fps_display);
    g_font.Draw(fps_buf, 8, screen_size.y - 8,
      kTextOriginTop, kTextAlignmentLeft,
      kDrawBlendingModeColorize, kFilterNearest,
      Rgba(255, 255, 255));

    g_font.Draw("WASD moves, mouse looks around, F12 saves a screenshot", 8, 8,
      kTextOriginBottom, kTextAlignmentLeft,
      kDrawBlendingModeColorize, kFilterNearest,
      Rgba(255, 255, 255));

    if (IsKeyDownward(kKeyF12)) {
      // Screenshot reads the frame before it is shown and hands it over as a
      // software sprite, which knows how to save itself as png or tga.
      Sprite frame = Screenshot();
      if (frame.Width() > 0) {
        frame.Save("screenshot.png");
      }
    }

    ShowFrame();
    Clear();
  }

  ReleaseMouse();
}
