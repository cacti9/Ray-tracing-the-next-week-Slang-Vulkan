#pragma once

#include "hittable.h"
#include "material.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

struct RtCpuBuilder {
  std::vector<Hittable>& hittables;
  std::vector<Material>& materials;

  RtCpuBuilder(std::vector<Hittable>& hittables_, std::vector<Material>& materials_)
      : hittables(hittables_), materials(materials_) {}

  uint32_t addLambertian(glm::vec<3, precision_type> albedo) {
    return addMaterial(MaterialType::Lambertian, Lambertian{.albedo = albedo});
  }

  uint32_t addMetal(glm::vec<3, precision_type> albedo, precision_type fuzz) {
    return addMaterial(MaterialType::Metal, Metal{.albedo = albedo, .fuzz = fuzz});
  }

  uint32_t addDielectric(precision_type refractionIndex) {
    return addMaterial(MaterialType::Dielectric, Dielectric{.refraction_index = refractionIndex});
  }

  void addStaticSphere(glm::vec<3, precision_type> static_center, precision_type radius, uint32_t materialIndex) {
    const precision_type clampedRadius = std::max(precision_type(0), radius);
    Sphere sphere{
      .center = {static_center, glm::vec<3, precision_type>(0, 0, 0), 0}, .radius = clampedRadius, .material_index = materialIndex
    };
    addHittable(HittableType::Sphere, sphere_bbox(static_center, clampedRadius), sphere);
  }

  void addMovingSphere(
    glm::vec<3, precision_type> center1, glm::vec<3, precision_type> center2, precision_type radius, uint32_t materialIndex
  ) {
    const precision_type clampedRadius = std::max(precision_type(0), radius);
    Sphere sphere{.center = {center1, center2 - center1, 0}, .radius = clampedRadius, .material_index = materialIndex};
    addHittable(HittableType::Sphere, Aabb(sphere_bbox(center1, clampedRadius), sphere_bbox(center2, clampedRadius)), sphere);
  }

private:
  template <typename Target, typename Payload> static void packPayload(Target& target, const Payload& payload) {
    static_assert(std::is_trivially_copyable_v<Payload>);
    assert(sizeof(payload) <= target.data.size() * sizeof(uint32_t));
    std::memcpy(target.data.data(), &payload, sizeof(payload));
  }

  template <typename Payload> uint32_t addMaterial(MaterialType type, const Payload& payload) {
    Material material{.type = type};
    packPayload(material, payload);
    materials.push_back(material);
    return static_cast<uint32_t>(materials.size() - 1);
  }

  template <typename Payload> void addHittable(HittableType type, const Aabb& bbox, const Payload& payload) {
    Hittable hittable{.type = type, .bbox = bbox};
    packPayload(hittable, payload);
    hittables.push_back(hittable);
  }

  static Aabb sphere_bbox(glm::vec<3, precision_type> center, precision_type radius) {
    const glm::vec<3, precision_type> rvec(radius, radius, radius);
    return Aabb(center - rvec, center + rvec);
  }
};

struct BvhBuilder {
  std::vector<Hittable>& hittables;

  explicit BvhBuilder(std::vector<Hittable>& hittables_) : hittables(hittables_) {}

  void build() {
    if (hittables.empty()) {
      return;
    }

    std::vector<uint32_t> hittableIndices(hittables.size());
    for (uint32_t i = 0; i < hittableIndices.size(); i++) {
      hittableIndices[i] = i;
    }

    (void)build(hittableIndices, 0, hittableIndices.size());
  }

private:
  uint32_t addBvhNode(uint32_t left, uint32_t right, const Aabb& bbox) {
    BvhNode node(left, right);
    Hittable hittable{.type = HittableType::BvhNode, .bbox = bbox};
    std::memcpy(hittable.data.data(), &node, sizeof(node));
    hittables.push_back(hittable);
    return static_cast<uint32_t>(hittables.size() - 1);
  }

  uint32_t build(std::vector<uint32_t>& hittableIndices, size_t start, size_t end) {
    Aabb spanBox;
    for (size_t objectIndex = start; objectIndex < end; objectIndex++) {
      spanBox = Aabb(spanBox, hittables[hittableIndices[objectIndex]].bbox);
    }

    const int axis = spanBox.longest_axis();
    std::sort(hittableIndices.begin() + start, hittableIndices.begin() + end, [&](uint32_t lhs, uint32_t rhs) {
      return BvhNode::box_compare(hittables[lhs].bbox, hittables[rhs].bbox, axis);
    });

    const size_t objectSpan = end - start;
    if (objectSpan == 1) {
      return hittableIndices[start];
    }
    if (objectSpan == 2) {
      return addBvhNode(hittableIndices[start], hittableIndices[start + 1], spanBox);
    }

    const size_t mid = start + objectSpan / 2;
    const uint32_t left = build(hittableIndices, start, mid);
    const uint32_t right = build(hittableIndices, mid, end);
    return addBvhNode(left, right, spanBox);
  }
};
