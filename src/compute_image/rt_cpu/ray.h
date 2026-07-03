#pragma once

#include "renderer_types.h"

#include <glm/glm.hpp>

struct ray {
  glm::vec<3, precision_type> orig;
  glm::vec<3, precision_type> dir;
  precision_type tm;
};
