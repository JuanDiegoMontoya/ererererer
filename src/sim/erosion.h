#pragma once
#include "../macros.h"
#include "../gfx/renderer.h"
#include <cstdint>
#include <glm/vec2.hpp>

namespace Erosion
{
  struct Particle
  {
    glm::vec2 pos{};
    glm::vec2 vel{};

  };

  class Simulation
  {
  public:
    Simulation(uint32_t width, uint32_t height);
    ~Simulation();

    void Init(uint64_t seed);
    void Update(double dt);
    [[nodiscard]] GFX::Heightmap GetHeightmap() const;

    NOCOPY_NOMOVE(Simulation)

  private:
    uint32_t width;
    uint32_t height;
    uint32_t textureA;
    uint32_t textureB;
  };
}