#pragma once

#include "renderer_types.h"

#include <array>
#include <cstdint>

#include <glm/glm.hpp>

enum class MaterialType : uint32_t {
  Lambertian = 0,
  Metal,
  Dielectric,
};

constexpr uint32_t MATERIAL_DATA_SIZE = RT_PRECISION_IS_DOUBLE ? 8 : 4;
constexpr uint32_t MATERIAL_DATA_BYTES = MATERIAL_DATA_SIZE * sizeof(uint32_t);

struct Material {
  MaterialType type;
  std::array<uint32_t, MATERIAL_DATA_SIZE> data;
};

struct Lambertian {
  uint32_t texture_index;
};

struct Metal {
  glm::vec<3, precision_type> albedo;
  precision_type fuzz;
};

struct Dielectric {
  precision_type refraction_index;
};
