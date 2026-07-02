#pragma once

#include <array>
#include <cstdint>
#include <glm/fwd.hpp>
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

using precision_type = float;
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

enum class HittableType : uint32_t {
  Sphere = 0,
};
constexpr uint32_t HITTABLE_DATA_SIZE = 16;
struct Hittable {
  HittableType type;
  std::array<uint32_t, HITTABLE_DATA_SIZE> data;
};
// corresponds to Hittable.data
struct Sphere {
  glm::vec<3, precision_type> center;
  precision_type radius;
  uint32_t material_index;
};

enum class MaterialType : uint32_t {
  Lambertian = 0,
  Metal,
  Dielectric,
};
constexpr uint32_t MATERIAL_DATA_SIZE = 8;
constexpr uint32_t MATERIAL_DATA_BYTES = MATERIAL_DATA_SIZE * sizeof(uint32_t);
struct Material {
  MaterialType type;
  std::array<uint32_t, MATERIAL_DATA_SIZE> data;
};
struct Lambertian {
  glm::vec<3, precision_type> albedo;
};
struct Metal {
  glm::vec<3, precision_type> albedo;
  precision_type fuzz;
};
struct Dielectric {
  precision_type refraction_index;
};

struct ShaderLoader {
  static std::vector<char> readFile(const std::string& filename);
  [[nodiscard]] static vk::raii::ShaderModule createShaderModule(vk::raii::Device const& device, const std::string& filename);
};
