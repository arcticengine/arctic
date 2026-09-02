// The OpenGL wrappers and their bookkeeping of the GL state: GlProgram,
// GlTexture2D, GlFramebuffer, GlBuffer, GlTextureCache.
#define TEST_NO_MAIN
#include "test_helpers.h"

// ---------------------------------------------------------------------------
// GL wrapper regression tests. Each of these once printed "[BUG]" from the
// benchmark project; here they check the real GL state instead of printing.
// ---------------------------------------------------------------------------

namespace {

// The smallest program with one float and one int uniform, both of which
// contribute to the color so that no driver optimizes them away.
const char *kRegressionVertexShader = R"SHADER(
#ifdef GL_ES
precision mediump float;
#endif
attribute vec2 vPosition;
void main() {
  gl_Position = vec4(vPosition, 0.0, 1.0);
}
)SHADER";

const char *kRegressionFragmentShader = R"SHADER(
#ifdef GL_ES
precision lowp float;
#endif
uniform float u_test;
uniform int u_count;
void main() {
  gl_FragColor = vec4(u_test, float(u_count) * 0.01, 0.0, 1.0);
}
)SHADER";

GLint CurrentGlProgramId() {
  GLint program_id = 0;
  glGetIntegerv(GL_CURRENT_PROGRAM, &program_id);
  return program_id;
}

GLint BoundColorAttachmentName() {
  GLint name = 0;
  glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);
  return name;
}

}  // namespace

// Once linked, a program has no use for its shader objects. Leaving them
// attached keeps them alive for as long as the program lives and leaks them
// for good when the program is deleted with the shaders still attached.
void test_gl_program_detaches_shaders_after_link() {
  if (arctic::GetEngine()->IsSoftwareOnly()) {
    TEST_MSG("skipped: this run has no OpenGL context");
    return;
  }
  GlProgram program;
  program.Create(kRegressionVertexShader, kRegressionFragmentShader);
  program.Bind();
  const GLint program_id = CurrentGlProgramId();
  TEST_CHECK_(program_id != 0, "Bind() left no program current");

  GLint attached = -1;
  glGetProgramiv(program_id, GL_ATTACHED_SHADERS, &attached);
  TEST_CHECK_(attached == 0,
      "%d shader object(s) are still attached to the linked program",
      static_cast<int>(attached));
  glUseProgram(0);
}

// A second SetUniform with the same name has to replace the first value.
// The table used to be filled with unordered_map::insert, which keeps the
// old value and silently drops every update.
void test_gl_uniforms_table_overwrites_a_value() {
  if (arctic::GetEngine()->IsSoftwareOnly()) {
    TEST_MSG("skipped: this run has no OpenGL context");
    return;
  }
  GlProgram program;
  program.Create(kRegressionVertexShader, kRegressionFragmentShader);
  program.Bind();
  const GLint program_id = CurrentGlProgramId();
  const GLint float_location = glGetUniformLocation(program_id, "u_test");
  const GLint int_location = glGetUniformLocation(program_id, "u_count");
  TEST_CHECK_(float_location >= 0 && int_location >= 0,
      "the shader lost a uniform (u_test at %d, u_count at %d)",
      static_cast<int>(float_location), static_cast<int>(int_location));

  UniformsTable table;
  table.SetUniform(std::string("u_test"), 1.0f);
  table.SetUniform(std::string("u_count"), 3);
  table.Apply(program);
  float first_float = -1.0f;
  GLint first_int = -1;
  glGetUniformfv(program_id, float_location, &first_float);
  glGetUniformiv(program_id, int_location, &first_int);
  TEST_CHECK_(first_float == 1.0f && first_int == 3,
      "the first values did not reach the GPU (%.1f, %d), so the update "
      "below proves nothing", first_float, static_cast<int>(first_int));

  table.SetUniform(std::string("u_test"), 99.0f);
  table.SetUniform(std::string("u_count"), 7);
  TEST_CHECK_(table.Size() == 2,
      "the table holds %d entries for two names",
      static_cast<int>(table.Size()));
  table.Apply(program);
  float second_float = -1.0f;
  GLint second_int = -1;
  glGetUniformfv(program_id, float_location, &second_float);
  glGetUniformiv(program_id, int_location, &second_int);
  TEST_CHECK_(second_float == 99.0f,
      "SetUniform(\"u_test\", 99.0f) was ignored, the GPU still has %.1f",
      second_float);
  TEST_CHECK_(second_int == 7,
      "SetUniform(\"u_count\", 7) was ignored, the GPU still has %d",
      static_cast<int>(second_int));
  glUseProgram(0);
}

// Bind(slot) promises that the slot is the active texture unit afterwards,
// because the caller goes on to call glTexParameteri or glTexSubImage2D on
// it. A cache hit on the texture used to skip glActiveTexture along with
// glBindTexture, and the following upload went to whatever unit was active.
void test_gl_texture2d_bind_hit_still_activates_the_slot() {
  if (arctic::GetEngine()->IsSoftwareOnly()) {
    TEST_MSG("skipped: this run has no OpenGL context");
    return;
  }
  GlTexture2D texture_a;
  texture_a.Create(1, 1);
  GlTexture2D texture_b;
  texture_b.Create(1, 1);

  texture_a.Bind(0);
  texture_b.Bind(1);
  GLint active = 0;
  glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
  TEST_CHECK_(active == GL_TEXTURE1,
      "Bind(1) left unit %d active instead of 1, so the hit below proves "
      "nothing", static_cast<int>(active - GL_TEXTURE0));

  // texture_a is already cached in slot 0: this is the cache hit.
  texture_a.Bind(0);
  glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
  TEST_CHECK_(active == GL_TEXTURE0,
      "a cache hit in Bind(0) left unit %d active",
      static_cast<int>(active - GL_TEXTURE0));
  GLint bound = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
  TEST_CHECK_(static_cast<GLuint>(bound) == texture_a.texture_id(),
      "unit 0 holds texture %d instead of %u",
      static_cast<int>(bound), texture_a.texture_id());
}

// Drivers hand out freed names again. A texture that was deleted while the
// bind cache still remembered it made the cache report a hit for the next
// texture with the same name, and that texture was never bound at all.
void test_gl_texture2d_forgets_a_deleted_name() {
  if (arctic::GetEngine()->IsSoftwareOnly()) {
    TEST_MSG("skipped: this run has no OpenGL context");
    return;
  }
  // Create() on a live texture: the old name is deleted inside.
  GlTexture2D texture;
  texture.Create(1, 1);
  texture.Bind(0);
  const GLuint old_id = texture.texture_id();
  texture.Create(2, 2);
  GLint active = 0;
  glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
  TEST_CHECK_(active == GL_TEXTURE0,
      "Create() bound to unit %d rather than 0",
      static_cast<int>(active - GL_TEXTURE0));
  GLint bound = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
  TEST_CHECK_(static_cast<GLuint>(bound) == texture.texture_id(),
      "after Create() unit 0 holds texture %d instead of %u (the old name "
      "was %u, %s)", static_cast<int>(bound), texture.texture_id(), old_id,
      old_id == texture.texture_id() ? "reused" : "not reused");

  // The destructor: the name dies with the object and another object gets it.
  GLuint dead_id = 0;
  {
    GlTexture2D short_lived;
    short_lived.Create(1, 1);
    short_lived.Bind(0);
    dead_id = short_lived.texture_id();
  }
  GlTexture2D successor;
  successor.Create(1, 1);
  successor.Bind(0);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
  TEST_CHECK_(static_cast<GLuint>(bound) == successor.texture_id(),
      "after a texture died unit 0 holds %d instead of %u (the dead name "
      "was %u, %s)", static_cast<int>(bound), successor.texture_id(),
      dead_id, dead_id == successor.texture_id() ? "reused" : "not reused");
}

// The same story for framebuffers: a stale name in the cache and Create()
// attaches the color texture to whatever framebuffer GL has bound, which
// after glDeleteFramebuffers is the default one.
void test_gl_framebuffer_forgets_a_deleted_name() {
  if (arctic::GetEngine()->IsSoftwareOnly()) {
    TEST_MSG("skipped: this run has no OpenGL context");
    return;
  }
  GlTexture2D texture_a;
  texture_a.Create(1, 1);
  GlTexture2D texture_b;
  texture_b.Create(1, 1);

  // Create() on a live framebuffer.
  GlFramebuffer framebuffer;
  framebuffer.Create(texture_a);
  framebuffer.Bind();
  GLint bound_before = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound_before);
  TEST_CHECK_(bound_before != 0, "the first Create() bound nothing");
  framebuffer.Create(texture_b);
  GLint bound_after = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound_after);
  TEST_CHECK_(bound_after != 0,
      "after the second Create() the default framebuffer is bound (the "
      "first one was %d)", static_cast<int>(bound_before));
  TEST_CHECK_(static_cast<GLuint>(BoundColorAttachmentName())
          == texture_b.texture_id(),
      "the bound framebuffer carries texture %d, not the new %u",
      static_cast<int>(BoundColorAttachmentName()), texture_b.texture_id());
  GlFramebuffer::BindDefault();

  // The destructor.
  {
    GlFramebuffer short_lived;
    short_lived.Create(texture_a);
    short_lived.Bind();
  }
  GlFramebuffer successor;
  successor.Create(texture_b);
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound_after);
  TEST_CHECK_(bound_after != 0,
      "after a framebuffer died Create() left the default one bound");
  TEST_CHECK_(static_cast<GLuint>(BoundColorAttachmentName())
          == texture_b.texture_id(),
      "the successor framebuffer carries texture %d, not %u",
      static_cast<int>(BoundColorAttachmentName()), texture_b.texture_id());
  GlFramebuffer::BindDefault();
}

// And for vertex buffers, where a false hit means SetData fills buffer 0.
void test_gl_buffer_forgets_a_deleted_name() {
  if (arctic::GetEngine()->IsSoftwareOnly()) {
    TEST_MSG("skipped: this run has no OpenGL context");
    return;
  }
  GlBuffer buffer;
  buffer.Create();
  buffer.Bind(GL_ARRAY_BUFFER);
  const GLuint old_id = buffer.buffer_id();
  buffer.Create();
  buffer.Bind(GL_ARRAY_BUFFER);
  GLint bound = 0;
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bound);
  TEST_CHECK_(static_cast<GLuint>(bound) == buffer.buffer_id(),
      "after Create() GL_ARRAY_BUFFER holds %d instead of %u (the old name "
      "was %u, %s)", static_cast<int>(bound), buffer.buffer_id(), old_id,
      old_id == buffer.buffer_id() ? "reused" : "not reused");

  GLuint dead_id = 0;
  {
    GlBuffer short_lived;
    short_lived.Create();
    short_lived.Bind(GL_ARRAY_BUFFER);
    dead_id = short_lived.buffer_id();
  }
  GlBuffer successor;
  successor.Create();
  successor.Bind(GL_ARRAY_BUFFER);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bound);
  TEST_CHECK_(static_cast<GLuint>(bound) == successor.buffer_id(),
      "after a buffer died GL_ARRAY_BUFFER holds %d instead of %u (the dead "
      "name was %u, %s)", static_cast<int>(bound), successor.buffer_id(),
      dead_id, dead_id == successor.buffer_id() ? "reused" : "not reused");
  GlBuffer::BindDefault(GL_ARRAY_BUFFER);
}

void test_gl_texture_cache_identity_and_white() {
  Sprite spr;
  spr.Create(4, 4);
  spr.Clear(Rgba(40, 50, 60, 200));
  const char *name = "FbBridgeCache_Id.png";
  spr.Save(name);
  if (!TEST_CHECK_(DoesFileExist(name) == kTrivalentTrue,
      "The cache fixture PNG must have been written")) {
    return;
  }
  GlTextureCache cache;
  GlTextureCacheEntry a = cache.Load(name);
  GlTextureCacheEntry b = cache.Load(name);
  TEST_CHECK_(a.texture.get() != nullptr,
      "Load of an existing PNG must return a texture object");
  TEST_CHECK_(a.texture.get() == b.texture.get(),
      "Two Load calls of the same file must return the same pointer");
  TEST_CHECK_(a.has_alpha, "A pixel with alpha 200 must set has_alpha");
  GlTextureCacheEntry found = cache.Find(name);
  TEST_CHECK_(found.texture.get() == a.texture.get(),
      "Find must return the entry Load stored");
  std::shared_ptr<GlTexture2D> white = cache.White();
  TEST_CHECK_(white.get() != nullptr, "White() must create a texture");
  TEST_CHECK_(white.get() != a.texture.get(),
      "White must not be the user file");
  std::shared_ptr<GlTexture2D> white2 = cache.White();
  TEST_CHECK_(white.get() == white2.get(),
      "White() must keep the same object");
  GlTextureCacheEntry miss = cache.Load("no_such_cache_tex_zzq.png");
  TEST_CHECK_(miss.texture.get() == nullptr,
      "A missing file must not invent a texture");
  std::remove(name);
}
