#include <iostream>
#include <format>
#include <stdexcept>
#include <algorithm>
#include <execution>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

#include <tiny_obj_loader.h>

#include "gfx/renderer.h"
#include "gfx/mesh.h"
#include "gfx/camera.h"
#include "engine.h"
#include "world.h"

struct WindowCreateInfo
{
  bool maximize{};
  bool decorate{};
  uint32_t width{};
  uint32_t height{};
};

GLFWwindow* CreateWindow(const WindowCreateInfo& createInfo)
{
  if (!glfwInit())
  {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  glfwSetErrorCallback([](int, const char* desc)
    {
      std::cout << std::format("GLFW error: {}\n", desc);
    });

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_MAXIMIZED, createInfo.maximize);
  glfwWindowHint(GLFW_DECORATED, createInfo.decorate);
  glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
  glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

  const GLFWvidmode* videoMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
  GLFWwindow* window = glfwCreateWindow(createInfo.width, createInfo.height, "ererererer", nullptr, nullptr);

  if (!window)
  {
    throw std::runtime_error("Failed to create window");
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  return window;
}

void InitOpenGL()
{
  int version = gladLoadGL(glfwGetProcAddress);
  if (version == 0)
  {
    throw std::runtime_error("Failed to initialize OpenGL");
  }
}


auto main() -> int
{
  GLFWwindow* window = CreateWindow({ .maximize = true, .decorate = true, .width = 1280, .height = 720 });

  InitOpenGL();

  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init();
  ImGui::StyleColorsDark();

  float SCALE = 1.0f;
  ImFontConfig cfg;
  cfg.SizePixels = 13 * SCALE;
  //ImGui::GetIO().Fonts->AddFontDefault(&cfg)->DisplayOffset.y = SCALE;
  ImGui::GetIO().Fonts->AddFontDefault(&cfg)->Scale = SCALE;

  int frameWidth, frameHeight;
  glfwGetFramebufferSize(window, &frameWidth, &frameHeight);

  glViewport(0, 0, frameWidth, frameHeight);

  World world;
  GFX::Renderer renderer;
  GFX::Mesh sphereMesh = GFX::LoadMesh("sphere.obj");
  GFX::Mesh cubeMesh = GFX::LoadMesh("cube.obj");
  world.sphereMeshHandle = renderer.GenerateMeshHandle(sphereMesh);
  world.cubeMeshHandle = renderer.GenerateMeshHandle(cubeMesh);
  world.io = &ImGui::GetIO();
  world.camera.proj = glm::perspective(glm::radians(70.0f), static_cast<float>(frameWidth) / frameHeight, 0.10f, 1000.0f);
  //world.camera.proj = glm::ortho(-50, 50, -50, 50, 1, 350);
  world.camera.viewInfo.position = { -5.5, 3, 0 };

  world.MakeBox({ 5, 5, 5 }, { 1, 2, 1 });

  double prevFrame = glfwGetTime();
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // escape toggles paused state if game is paused or unpaused
    if (world.io->KeysDownDuration[GLFW_KEY_ESCAPE] == 0.0f)
    {
      if (world.gameState == GameState::PAUSED)
      {
        world.gameState = GameState::UNPAUSED;
      }
      else if (world.gameState == GameState::UNPAUSED)
      {
        world.gameState = GameState::PAUSED;
      }
    }

    // disable mouse if game is unpaused
    if (world.gameState == GameState::UNPAUSED)
    {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else
    {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    double curFrame = glfwGetTime();
    double dt = curFrame - prevFrame;
    prevFrame = curFrame;

    if (world.gameState != GameState::UNPAUSED)
    {
      dt = 0;
    }

    switch (world.gameState)
    {
    case GameState::PAUSED:
    {
      ImGui::SetNextWindowPos(ImVec2(world.io->DisplaySize.x * 0.5f, world.io->DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
      ImGui::SetNextWindowSize(ImVec2(400, 400));
      ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration);

      if (ImGui::Button("Resume", { -1, 0 }))
      {
        world.gameState = GameState::UNPAUSED;
      }

      if (ImGui::Button("Quit", { -1, 0 }))
      {
        glfwSetWindowShouldClose(window, true);
      }

      ImGui::SetNextItemOpen(true);
      if (ImGui::TreeNode("Options"))
      {
        static float FoV = 70.0f;
        if (ImGui::SliderFloat("FoV", &FoV, 20, 120))
        {
          world.camera.proj = glm::perspective(glm::radians(FoV), static_cast<float>(frameWidth) / frameHeight, 0.10f, 1000.0f);
        }

        float sensTemp = world.mouseSensitivity * 100;
        if (ImGui::SliderFloat("Look sensitivity", &sensTemp, .01, 2))
        {
          world.mouseSensitivity = sensTemp / 100;
        }

        ImGui::TreePop();
      }

      ImGui::SetNextItemOpen(true);
      if (ImGui::TreeNode("Controls"))
      {
        ImGui::Text(
          "WASD       : move\n"
          "Mouse      : look\n"
          "Left Shift : fast movement\n"
          "Left Ctrl  : slow movement\n"
          "Escape     : pause/unpause"
        );
        ImGui::TreePop();
      }

      ImGui::End();
      break;
    }
    case GameState::UNPAUSED:
    {
      DEBUG_PRINT(UNPAUSED);

      auto& vi = world.camera.viewInfo;
      vi.yaw += world.io->MouseDelta.x * world.mouseSensitivity;
      vi.pitch = glm::clamp(vi.pitch - world.io->MouseDelta.y * world.mouseSensitivity, glm::radians(-89.0f), glm::radians(89.0f));

      float speed = 10.0f;
      if (world.io->KeysDown[GLFW_KEY_LEFT_SHIFT])
        speed = 100.0f;
      if (world.io->KeysDown[GLFW_KEY_LEFT_CONTROL])
        speed = 1.0f;

      glm::vec3 movement{ 0 };

      const glm::vec3 forward = world.camera.viewInfo.GetForwardDir();
      const glm::vec3 up = { 0, 1, 0 };
      const glm::vec3 right = glm::cross(forward, up);

      if (world.io->KeysDown[GLFW_KEY_W])
      {
        movement += forward;
      }
      if (world.io->KeysDown[GLFW_KEY_A])
      {
        movement -= right;
      }
      if (world.io->KeysDown[GLFW_KEY_S])
      {
        movement -= forward;
      }
      if (world.io->KeysDown[GLFW_KEY_D])
      {
        movement += right;
      }

      movement *= dt * speed;
      world.camera.viewInfo.position += movement;

      break;
    }
    default:
      assert(0 && "Illegal gamestate!");
      break;
    }
    
    // draw everything
    auto& objects = world.entityManager.GetObjects();
    renderer.BeginDraw(objects.size());
    std::for_each(std::execution::par_unseq, objects.begin(), objects.end(), [&renderer](const auto& obj)
      {
        renderer.Submit(obj.transform, obj.mesh, obj.renderable);
      });
    renderer.EndDraw(world.camera, dt);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGui::EndFrame();

    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwTerminate();
  return 0;
}