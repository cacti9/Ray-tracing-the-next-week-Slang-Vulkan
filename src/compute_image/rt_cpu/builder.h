#pragma once

#include "hittable.h"
#include "material.h"
#include "renderer_types.h"
#include "texture.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/gtx/hash.hpp>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct PerlinBuilder {
  static Perlin build(uint32_t seedOffset = 0) {
    Perlin perlin{};
    std::mt19937 randomEngine{1 + seedOffset};
    std::uniform_real_distribution<precision_type> unitDistribution{-1.0, 1.0};

    for (uint32_t i = 0; i < PERLIN_POINT_COUNT; i++) {
      glm::vec<3, precision_type> temp = {
        unitDistribution(randomEngine), unitDistribution(randomEngine), unitDistribution(randomEngine)
      };
      temp = glm::normalize(temp);
      perlin.randvec_unpacked[3 * i] = temp.x;
      perlin.randvec_unpacked[3 * i + 1] = temp.y;
      perlin.randvec_unpacked[3 * i + 2] = temp.z;
    }

    generatePerm(perlin.perm_x, randomEngine);
    generatePerm(perlin.perm_y, randomEngine);
    generatePerm(perlin.perm_z, randomEngine);

    return perlin;
  }

private:
  static void generatePerm(std::array<uint32_t, PERLIN_POINT_COUNT>& p, std::mt19937& randomEngine) {
    for (uint32_t i = 0; i < PERLIN_POINT_COUNT; i++) {
      p[i] = i;
    }
    permute(p, randomEngine);
  }

  static void permute(std::array<uint32_t, PERLIN_POINT_COUNT>& p, std::mt19937& randomEngine) {
    for (uint32_t i = PERLIN_POINT_COUNT - 1; i > 0; i--) {
      std::uniform_int_distribution<uint32_t> targetDistribution{0, i};
      const uint32_t target = targetDistribution(randomEngine);
      std::swap(p[i], p[target]);
    }
  }
};

struct RtCpuBuilder {
  std::vector<Hittable>& hittables;
  std::vector<Material>& materials;
  std::vector<Texture>& textures;
  std::vector<std::string>& imageTexturePaths;
  std::vector<Perlin>& perlins;

  RtCpuBuilder(
    std::vector<Hittable>& hittables_, std::vector<Material>& materials_, std::vector<Texture>& textures_,
    std::vector<std::string>& imageTexturePaths_, std::vector<Perlin>& perlins_
  )
      : hittables(hittables_), materials(materials_), textures(textures_), imageTexturePaths(imageTexturePaths_),
        perlins(perlins_) {}

  uint32_t addLambertian(glm::vec<3, precision_type> albedo) { return addLambertian(addSolidColor(albedo)); }

  uint32_t addLambertian(uint32_t textureIndex) {
    return addMaterial(MaterialType::Lambertian, Lambertian{.texture_index = textureIndex});
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

  uint32_t addSolidColor(glm::vec<3, precision_type> albedo) {
    return addTexture(TextureType::SolidColor, SolidColor{.albedo = albedo});
  }

  uint32_t addCheckerTexture(precision_type scale, uint32_t evenTextureIndex, uint32_t oddTextureIndex) {
    return addTexture(
      TextureType::Checker,
      CheckerTexture{.inv_scale = 1.0f / scale, .even_texture_index = evenTextureIndex, .odd_texture_index = oddTextureIndex}
    );
  }

  uint32_t addCheckerTexture(precision_type scale, glm::vec<3, precision_type> even, glm::vec<3, precision_type> odd) {
    return addCheckerTexture(scale, addSolidColor(even), addSolidColor(odd));
  }

  uint32_t addImageTexture(std::string path) {
    imageTexturePaths.push_back(std::move(path));
    return addTexture(TextureType::Image, ImageTexture{.image_index = static_cast<uint32_t>(imageTexturePaths.size() - 1)});
  }

  uint32_t addNoiseTexture(precision_type scale) {
    if (perlins.empty()) {
      perlins.push_back(PerlinBuilder::build());
    }
    return addTexture(TextureType::Noise, NoiseTexture{.scale = scale});
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

  template <typename Payload> uint32_t addTexture(TextureType type, const Payload& payload) {
    Texture texture{.type = type};
    packPayload(texture, payload);
    textures.push_back(texture);
    return static_cast<uint32_t>(textures.size() - 1);
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
