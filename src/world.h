#pragma once

#include <imgui.h>
#include "gfx/camera.h"
#include "engine.h"

#define DEBUG_PRINT 0
#if DEBUG_PRINT
#define DEBUG_PRINT(x) printf(#x "\n");
#else
#define DEBUG_PRINT(x)
#endif

enum class GameState
{
  PAUSED,
  UNPAUSED,
};

struct World
{
  float mouseSensitivity = 0.002f;
  GameState gameState = GameState::PAUSED;

  ImGuiIO* io{};
  GFX::Camera camera;
  Game::EntityManager entityManager;

  MeshHandle sphereMeshHandle;
  MeshHandle cubeMeshHandle;

  Game::GameObject& MakeSphere(glm::vec3 pos, float scale)
  {
    Game::GameObject& obj = entityManager.GetObject(entityManager.CreateEntity());
    obj.transform.position = pos;
    obj.transform.scale = glm::vec3(scale);
    obj.mesh = sphereMeshHandle;
    obj.renderable.visible = true;
    return obj;
  }

  Game::GameObject& MakeBox(glm::vec3 pos, glm::vec3 halfExtents)
  {
    Game::GameObject& obj = entityManager.GetObject(entityManager.CreateEntity());
    obj.transform.position = pos;
    obj.transform.scale = glm::vec3(halfExtents);
    obj.mesh = cubeMeshHandle;
    obj.renderable.visible = true;
    return obj;
  }
};