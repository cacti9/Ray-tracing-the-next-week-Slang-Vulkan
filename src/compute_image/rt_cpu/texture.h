#pragma once

#include "renderer_types.h"

#include <array>
#include <cstdint>

#include <glm/glm.hpp>

enum class TextureType : uint32_t {
  SolidColor = 0,
  Checker,
  Image,
  Noise,
};

constexpr uint32_t TEXTURE_DATA_SIZE = RT_PRECISION_IS_DOUBLE ? 8 : 4;
constexpr uint32_t TEXTURE_DATA_BYTES = TEXTURE_DATA_SIZE * sizeof(uint32_t);
constexpr uint32_t PERLIN_POINT_COUNT = 256;

struct Texture {
  TextureType type;
  std::array<uint32_t, TEXTURE_DATA_SIZE> data;
};

struct SolidColor {
  glm::vec<3, precision_type> albedo;
};

struct CheckerTexture {
  precision_type inv_scale;
  uint32_t even_texture_index;
  uint32_t odd_texture_index;
};

struct ImageTexture {
  uint32_t image_index;
};

struct Perlin {
  std::array<precision_type, PERLIN_POINT_COUNT * 3> randvec_unpacked;
  std::array<uint32_t, PERLIN_POINT_COUNT> perm_x;
  std::array<uint32_t, PERLIN_POINT_COUNT> perm_y;
  std::array<uint32_t, PERLIN_POINT_COUNT> perm_z;
};

struct NoiseTexture {
  precision_type scale;
};
