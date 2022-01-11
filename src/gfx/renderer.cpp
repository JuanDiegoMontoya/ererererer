#include "renderer.h"

#include <sstream>
#include <iostream>
#include <format>
#include <concepts>
#include <atomic>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/gl.h>

#include "shader.h"
#include "mesh.h"
#include "camera.h"
#include "components.h"

static void GLAPIENTRY glErrorCallback(
  GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  [[maybe_unused]] GLsizei length,
  const GLchar* message,
  [[maybe_unused]] const void* userParam)
{
  // ignore insignificant error/warning codes
  if (id == 131169 || id == 131185 || id == 131218 || id == 131204 || id == 0
    )//|| id == 131188 || id == 131186)
    return;

  std::stringstream errStream;
  errStream << "OpenGL Debug message (" << id << "): " << message << '\n';

  switch (source)
  {
  case GL_DEBUG_SOURCE_API:             errStream << "Source: API"; break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   errStream << "Source: Window Manager"; break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER: errStream << "Source: Shader Compiler"; break;
  case GL_DEBUG_SOURCE_THIRD_PARTY:     errStream << "Source: Third Party"; break;
  case GL_DEBUG_SOURCE_APPLICATION:     errStream << "Source: Application"; break;
  case GL_DEBUG_SOURCE_OTHER:           errStream << "Source: Other"; break;
  }

  errStream << '\n';

  switch (type)
  {
  case GL_DEBUG_TYPE_ERROR:               errStream << "Type: Error"; break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: errStream << "Type: Deprecated Behaviour"; break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  errStream << "Type: Undefined Behaviour"; break;
  case GL_DEBUG_TYPE_PORTABILITY:         errStream << "Type: Portability"; break;
  case GL_DEBUG_TYPE_PERFORMANCE:         errStream << "Type: Performance"; break;
  case GL_DEBUG_TYPE_MARKER:              errStream << "Type: Marker"; break;
  case GL_DEBUG_TYPE_PUSH_GROUP:          errStream << "Type: Push Group"; break;
  case GL_DEBUG_TYPE_POP_GROUP:           errStream << "Type: Pop Group"; break;
  case GL_DEBUG_TYPE_OTHER:               errStream << "Type: Other"; break;
  }

  errStream << '\n';

  switch (severity)
  {
  case GL_DEBUG_SEVERITY_HIGH:
    errStream << "Severity: high";
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    errStream << "Severity: medium";
    break;
  case GL_DEBUG_SEVERITY_LOW:
    errStream << "Severity: low";
    break;
  case GL_DEBUG_SEVERITY_NOTIFICATION:
    errStream << "Severity: notification";
    break;
  }

  std::cout << errStream.str() << '\n';
}

namespace GFX
{
  namespace
  {
    struct RenderTuple
    {
      glm::mat4 model;
      MeshHandle mesh;
      Renderable renderable;
    };

    constexpr int gl_index_type()
    {
      if constexpr (std::same_as<index_t, uint32_t>)
        return GL_UNSIGNED_INT;
      if constexpr (std::same_as<index_t, uint16_t>)
        return GL_UNSIGNED_SHORT;
      if constexpr (std::same_as<index_t, uint8_t>)
        return GL_UNSIGNED_BYTE;

      return 0;
    }
  }

  struct RendererImpl
  {
    ////////////////////////////////////////////////////////
    // objects
    ////////////////////////////////////////////////////////
    GLuint emptyVao{};
    GLuint standardVao{};
    Shader standardShader{};
    Shader environmentShader{};
    Shader heightmapShader{};

    std::vector<RenderTuple> renderables;
    glm::vec3 sunDir = { 0, -1, 0 };
    float blendDay = 0;
    double gTime = 10.0;
    std::atomic_uint32_t drawIndex{ 0 };

    ////////////////////////////////////////////////////////
    // functions
    ////////////////////////////////////////////////////////
    RendererImpl()
    {
      // for bufferless drawing
      glCreateVertexArrays(1, &emptyVao);

      // standard vertex format with pos, normal, and texcoord
      glCreateVertexArrays(1, &standardVao);
      glEnableVertexArrayAttrib(standardVao, 0);
      glEnableVertexArrayAttrib(standardVao, 1);
      glEnableVertexArrayAttrib(standardVao, 2);
      glVertexArrayAttribFormat(standardVao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
      glVertexArrayAttribFormat(standardVao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
      glVertexArrayAttribFormat(standardVao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texcoord));
      glVertexArrayAttribBinding(standardVao, 0, 0);
      glVertexArrayAttribBinding(standardVao, 1, 0);
      glVertexArrayAttribBinding(standardVao, 2, 0);

      standardShader = LoadVertexFragmentProgram("standard.vert.glsl", "standard.frag.glsl");
      environmentShader = LoadVertexFragmentProgram("environment.vert.glsl", "environment.frag.glsl");
      heightmapShader = LoadVertexFragmentProgram("heightmap.vert.glsl", "heightmap.frag.glsl");

#ifndef NDEBUG
      // enable debugging stuff
      glEnable(GL_DEBUG_OUTPUT);
      glDebugMessageCallback(glErrorCallback, NULL);
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
      glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

      GL_ResetState();
    }

    ~RendererImpl()
    {
      glDeleteVertexArrays(1, &emptyVao);
      glDeleteVertexArrays(1, &standardVao);
      // everything else is leaked because this class is instantiated once and destroyed when the program terminates
    }

    void GL_ResetState()
    {
      // texture unit and sampler bindings (first 8, hopefully more than we'll ever need)
      for (int i = 0; i < 8; i++)
      {
        glBindSampler(i, 0);
        glBindTextureUnit(i, 0);
      }
      glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

      // triangle winding
      glEnable(GL_CULL_FACE);
      glCullFace(GL_BACK);
      glFrontFace(GL_CCW);

      // depth test
      glEnable(GL_DEPTH_TEST);
      glDepthMask(GL_TRUE);
      glDepthFunc(GL_LEQUAL);

      // blending
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glBlendEquation(GL_FUNC_ADD);

      // buffer bindings
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
      glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0);
      glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);

      // vertex array binding
      glBindVertexArray(0);

      // shader program binding
      glUseProgram(0);

      // viewport+clipping
      glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

      // rasterizer
      glLineWidth(1.0f);
      glPointSize(1.0f);
      glDisable(GL_SCISSOR_TEST);
      //glViewport(0, 0, ..., ...);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

      // framebuffer
      glClearDepth(1.0);
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void BeginDraw(uint32_t numObjects)
    {
      drawIndex.store(0);
      renderables.resize(numObjects);
    }

    void Submit(const Transform& transform,
      const MeshHandle& mesh,
      const Renderable& renderable)
    {
      uint32_t myIndex = drawIndex.fetch_add(1);
      renderables[myIndex] = { transform.GetModel(), mesh, renderable };
    }

    void EndDraw(const Camera& camera, float dt)
    {
      //gTime += dt;
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glEnable(GL_FRAMEBUFFER_SRGB);

      DrawRenderables(camera);
      DrawEnvironment(camera);

      sunDir.y = -glm::sin(gTime / 10);
      sunDir.x = glm::cos(gTime / 10);
      sunDir.z = 0.8;
      sunDir = glm::normalize(sunDir);
      blendDay = glm::max(-sunDir.y * 2, 0.0f);
    }

    void DrawRenderables(const Camera& camera)
    {
      standardShader.Bind();
      standardShader.SetMat4("u_viewProj", camera.GetViewProj());
      standardShader.SetVec3("u_sunDir", sunDir);
      standardShader.SetFloat("u_blendDay", blendDay);
      glBindVertexArray(standardVao);

      // TODO: sort by mesh to reduce binding
      for (const auto& [model, mesh, renderable] : renderables)
      {
        if (!renderable.visible)
        {
          continue;
        }

        standardShader.SetMat4("u_model", model);
        standardShader.SetVec4("u_color", renderable.color);
        standardShader.SetVec3("u_glow", renderable.glow);
        glVertexArrayVertexBuffer(standardVao, 0, mesh.vertexBuffer, 0, sizeof(Vertex));
        glVertexArrayElementBuffer(standardVao, mesh.indexBuffer);
        glDrawElements(GL_TRIANGLES, mesh.count, gl_index_type(), nullptr);
      }

      renderables.clear();
    }

    void DrawEnvironment(const Camera& camera)
    {
      environmentShader.Bind();
      environmentShader.SetMat4("u_invViewProj", glm::inverse(camera.GetViewProj()));
      environmentShader.SetVec3("u_viewPos", camera.viewInfo.position);
      environmentShader.SetFloat("u_blendDay", blendDay);
      environmentShader.SetVec3("u_sunDir", sunDir);
      glBindVertexArray(emptyVao);
      glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void DrawHeightmap(const Camera& camera, Heightmap heightmap)
    {
      glm::mat4 model(1);
      model = glm::scale(model, glm::vec3(5));

      heightmapShader.Bind();
      heightmapShader.SetMat4("u_viewProj", camera.GetViewProj());
      heightmapShader.SetMat4("u_model", model);

      glBindVertexArray(emptyVao);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }
  };

  Renderer::Renderer()
  {
    impl_ = new RendererImpl;
  }

  Renderer::~Renderer()
  {
    delete impl_;
  }

  MeshHandle Renderer::GenerateMeshHandle(const Mesh& mesh)
  {
    MeshHandle handle;
    handle.count = mesh.vertices.size();
    
    glCreateBuffers(1, &handle.vertexBuffer);
    glNamedBufferStorage(handle.vertexBuffer, sizeof(Vertex) * mesh.vertices.size(), mesh.vertices.data(), 0);

    glCreateBuffers(1, &handle.indexBuffer);
    glNamedBufferStorage(handle.indexBuffer, sizeof(index_t) * mesh.indices.size(), mesh.indices.data(), 0);

    return handle;
  }

  void Renderer::BeginDraw(uint32_t numObjects)
  {
    impl_->BeginDraw(numObjects);
  }

  void Renderer::Submit(const Transform& transform,
    const MeshHandle& mesh,
    const Renderable& renderable)
  {
    impl_->Submit(transform, mesh, renderable);
  }

  void Renderer::EndDraw(const Camera& camera, float dt)
  {
    impl_->EndDraw(camera, dt);
  }

  void Renderer::DrawHeightmap(const Camera& camera, Heightmap heightmap)
  {
    impl_->DrawHeightmap(camera, heightmap);
  }
}