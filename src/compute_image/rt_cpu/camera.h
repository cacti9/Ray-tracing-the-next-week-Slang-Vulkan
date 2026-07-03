#pragma once

#include <glm/glm.hpp>

struct CameraSettings {
  glm::dvec3 look_from{13, 2, 3};
  glm::dvec3 look_at{0, 0, 0};
  glm::dvec3 vup{0, 1, 0};
  double vfov = 20.0;
  double defocus_angle = 0.0;
  double focus_dist = 10.0;
};
