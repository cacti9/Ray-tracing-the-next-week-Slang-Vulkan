#pragma once

#include <cstdint>
#include <glm/fwd.hpp>
#include <limits>
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
constexpr int MAX_FRAMES_IN_FLIGHT = 1;
constexpr uint32_t MAX_IMAGE_TEXTURES = 16;

using precision_type = float;
constexpr precision_type INF = std::numeric_limits<precision_type>::infinity();

struct UniformBufferObject {
  glm::vec<4, precision_type> pixel00_loc;
  glm::vec<4, precision_type> pixel_delta_u;
  glm::vec<4, precision_type> pixel_delta_v;
  glm::vec<4, precision_type> camera_center;
  glm::vec<4, precision_type> defocus_disk_u;
  glm::vec<4, precision_type> defocus_disk_v;
  glm::uvec2 renderExtent;
  uint32_t hittable_count;
  uint32_t max_depth;
  uint32_t samples_per_pixel;
  float defocus_angle;
};

struct ShaderLoader {
  static std::vector<char> readFile(const std::string& filename);
  [[nodiscard]] static vk::raii::ShaderModule createShaderModule(vk::raii::Device const& device, const std::string& filename);
};
