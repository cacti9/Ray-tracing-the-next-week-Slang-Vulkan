#pragma once

#include "renderer_types.h"

#include <array>
#include <cstdint>

#include <glm/glm.hpp>

enum class TextureType : uint32_t {
  SolidColor = 0,
  Checker,
  Image,
};

constexpr uint32_t TEXTURE_DATA_SIZE = 4;
constexpr uint32_t TEXTURE_DATA_BYTES = TEXTURE_DATA_SIZE * sizeof(uint32_t);

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
