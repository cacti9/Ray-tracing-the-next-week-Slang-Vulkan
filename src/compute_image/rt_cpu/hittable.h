#pragma once

#include "aabb.h"
#include "ray.h"

#include <array>
#include <cstdint>

enum class HittableType : uint32_t {
  BvhNode = 0,
  Sphere,
  Quad,
  HittableList,
  Translate,
  RotateY,
  ConstantMedium,
};

constexpr uint32_t HITTABLE_DATA_SIZE = RT_PRECISION_IS_DOUBLE ? 40 : 24;
constexpr uint32_t HITTABLE_DATA_BYTES = HITTABLE_DATA_SIZE * sizeof(uint32_t);

struct Hittable {
  HittableType type;
  Aabb bbox;
  std::array<uint32_t, HITTABLE_DATA_SIZE> data;
};

struct Sphere {
  ray center;
  precision_type radius;
  uint32_t material_index;
};

struct Quad {
  glm::vec<3, precision_type> Q;
  glm::vec<3, precision_type> u;
  glm::vec<3, precision_type> v;
  glm::vec<3, precision_type> w;
  glm::vec<3, precision_type> normal;
  precision_type D;
  uint32_t material_index;
};

struct BvhNode {
  uint32_t left;
  uint32_t right;

  BvhNode() = default;

  BvhNode(uint32_t left_, uint32_t right_) : left(left_), right(right_) {}

  static bool box_compare(const Aabb& a, const Aabb& b, int axis_index) {
    return a.axis_interval(axis_index).min < b.axis_interval(axis_index).min;
  }
};

struct HittableList {
  uint32_t first;
  uint32_t count;
};

struct Translate {
  uint32_t hittable_index;
  glm::vec<3, precision_type> offset;
};

struct RotateY {
  uint32_t hittable_index;
  precision_type sin_theta;
  precision_type cos_theta;
};

struct ConstantMedium {
  uint32_t boundary_hittable_index;
  precision_type neg_inv_density;
  uint32_t phase_function_material_index; // isotropic
};
