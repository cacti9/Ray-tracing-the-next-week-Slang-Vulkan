#pragma once

#include "aabb.h"
#include "ray.h"

#include <array>
#include <cstdint>

enum class HittableType : uint32_t {
  BvhNode = 0,
  Sphere,
};

constexpr uint32_t HITTABLE_DATA_SIZE = RT_PRECISION_IS_DOUBLE ? 20 : 16;
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

struct BvhNode {
  uint32_t left;
  uint32_t right;

  BvhNode() = default;

  BvhNode(uint32_t left_, uint32_t right_) : left(left_), right(right_) {}

  static bool box_compare(const Aabb& a, const Aabb& b, int axis_index) {
    return a.axis_interval(axis_index).min < b.axis_interval(axis_index).min;
  }
};
