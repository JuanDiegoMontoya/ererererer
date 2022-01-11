#pragma once

#include <cstdint>
#include "macros.h"

struct Transform;
struct MeshHandle;
struct Renderable;

namespace GFX
{
  struct Camera;
  struct Mesh;

  struct Heightmap
  {

  };

  class Renderer
  {
  public:
    Renderer();
    ~Renderer();

    NOCOPY_NOMOVE(Renderer)

    [[nodiscard]] MeshHandle GenerateMeshHandle(const Mesh& mesh);

    void BeginDraw(uint32_t numObjects);
    void Submit(const Transform& transform,
      const MeshHandle& mesh,
      const Renderable& renderable);
    void EndDraw(const Camera& camera, float dt);

    void DrawHeightmap(const Camera& camera, Heightmap heightmap);

  private:
    struct RendererImpl* impl_;
  };
}