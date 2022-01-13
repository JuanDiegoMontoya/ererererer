#include "erosion.h"
#include <glad/gl.h>
#include <memory>
#include <glm/glm.hpp>

namespace Erosion
{
  Simulation::Simulation(uint32_t w, uint32_t h)
    : width(w), height(h)
  {
    glCreateTextures(GL_TEXTURE_2D, 1, &textureA);
    glCreateTextures(GL_TEXTURE_2D, 1, &textureB);

    glTextureStorage2D(textureA, 1, GL_R32F, width, height);
    glTextureStorage2D(textureB, 1, GL_R32F, width, height);
  }

  Simulation::~Simulation()
  {
    glDeleteTextures(1, &textureB);
    glDeleteTextures(1, &textureA);
  }

  void Simulation::Init(uint64_t seed)
  {
    auto map = std::make_unique<float[]>(width * height);

    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
      {
        //map[x + y * width] = (x ^ y) & (~0u ^ 1) ? 1.0f : 0.0f;
        map[x + y * width] = glm::distance(glm::vec2(x, y), glm::vec2(height, width) / 2.0f) / 100;
        //map[x + y * width] = y / 100.0f;
      }
    }

    glTextureSubImage2D(textureA, 0, 0, 0, width, height, GL_RED, GL_FLOAT, map.get());
    glTextureSubImage2D(textureB, 0, 0, 0, width, height, GL_RED, GL_FLOAT, map.get());

    glTextureParameteri(textureA, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(textureA, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(textureB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(textureB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(textureA, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(textureA, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTextureParameteri(textureB, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTextureParameteri(textureB, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  }

  void Simulation::Update(double dt)
  {
  }

  GFX::Heightmap Simulation::GetHeightmap() const
  {
    return GFX::Heightmap{ width, height, textureA };
  }
}