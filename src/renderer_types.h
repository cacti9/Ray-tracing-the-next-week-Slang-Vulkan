#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 800;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

using precision_type = double;
struct UniformBufferObject {
  glm::vec<4, precision_type, glm::defaultp> pixel00_loc;
  glm::vec<4, precision_type, glm::defaultp> pixel_delta_u;
  glm::vec<4, precision_type, glm::defaultp> pixel_delta_v;
  glm::vec<4, precision_type, glm::defaultp> camera_center;
  glm::vec<4, precision_type, glm::defaultp> defocus_disk_u;
  glm::vec<4, precision_type, glm::defaultp> defocus_disk_v;
  glm::uvec4 renderExtent;
  float defocus_angle;
  float deltaTime;
};

struct ShaderLoader {
  static std::vector<char> readFile(const std::string& filename);
  [[nodiscard]] static vk::raii::ShaderModule createShaderModule(vk::raii::Device const& device, const std::string& filename);
};
