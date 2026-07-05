#pragma once

#include "hittable.h"
#include "material.h"
#include "renderer_types.h"
#include "texture.h"

#include <algorithm>
#include <cassert>
#include <cmath>
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
  std::vector<uint32_t>& rootHittables;

  RtCpuBuilder(
    std::vector<Hittable>& hittables_, std::vector<Material>& materials_, std::vector<Texture>& textures_,
    std::vector<std::string>& imageTexturePaths_, std::vector<Perlin>& perlins_, std::vector<uint32_t>& rootHittables_
  )
      : hittables(hittables_), materials(materials_), textures(textures_), imageTexturePaths(imageTexturePaths_), perlins(perlins_),
        rootHittables(rootHittables_) {}

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

  uint32_t addDiffuseLight(glm::vec<3, precision_type> emit) { return addDiffuseLight(addSolidColor(emit)); }

  uint32_t addDiffuseLight(uint32_t textureIndex) {
    return addMaterial(MaterialType::DiffuseLight, DiffuseLight{.texture_index = textureIndex});
  }

  uint32_t addIsotropic(glm::vec<3, precision_type> albedo) { return addIsotropic(addSolidColor(albedo)); }

  uint32_t addIsotropic(uint32_t texture_index) {
    return addMaterial(MaterialType::Isotropic, Isotropic{.texture_index = texture_index});
  }

  uint32_t
  addStaticSphere(glm::vec<3, precision_type> static_center, precision_type radius, uint32_t materialIndex, bool root = true) {
    const precision_type clampedRadius = std::max(precision_type(0), radius);
    Sphere sphere{
      .center = {static_center, glm::vec<3, precision_type>(0, 0, 0), 0}, .radius = clampedRadius, .material_index = materialIndex
    };
    return addHittable(HittableType::Sphere, sphere_bbox(static_center, clampedRadius), sphere, root);
  }

  uint32_t addMovingSphere(
    glm::vec<3, precision_type> center1, glm::vec<3, precision_type> center2, precision_type radius, uint32_t materialIndex,
    bool root = true
  ) {
    const precision_type clampedRadius = std::max(precision_type(0), radius);
    Sphere sphere{.center = {center1, center2 - center1, 0}, .radius = clampedRadius, .material_index = materialIndex};
    return addHittable(
      HittableType::Sphere, Aabb(sphere_bbox(center1, clampedRadius), sphere_bbox(center2, clampedRadius)), sphere, root
    );
  }

  uint32_t addQuad(
    glm::vec<3, precision_type> Q, glm::vec<3, precision_type> u, glm::vec<3, precision_type> v, uint32_t materialIndex,
    bool root = true
  ) {
    return addQuadNode(Q, u, v, materialIndex, root);
  }

  uint32_t addBox(glm::vec<3, precision_type> a, glm::vec<3, precision_type> b, uint32_t materialIndex, bool root = true) {
    return addBoxNode(a, b, materialIndex, root);
  }

  uint32_t addBox(
    glm::vec<3, precision_type> a, glm::vec<3, precision_type> b, precision_type angle, glm::vec<3, precision_type> offset,
    uint32_t materialIndex, bool root = true
  ) {
    const uint32_t box = addBoxNode(a, b, materialIndex, false);
    const uint32_t rotatedBox = addRotateY(box, angle, false);
    return addTranslate(rotatedBox, offset, root);
  }

  uint32_t addTranslate(uint32_t hittableIndex, glm::vec<3, precision_type> offset, bool root = true) {
    Translate translate{.hittable_index = hittableIndex, .offset = offset};
    return addHittable(HittableType::Translate, hittables[hittableIndex].bbox + offset, translate, root);
  }

  uint32_t addRotateY(uint32_t hittableIndex, precision_type angle, bool root = true) {
    constexpr precision_type pi = precision_type(3.1415926535897932385);
    const precision_type radians = angle * pi / precision_type(180);
    RotateY rotate{
      .hittable_index = hittableIndex,
      .sin_theta = std::sin(radians),
      .cos_theta = std::cos(radians),
    };
    return addHittable(HittableType::RotateY, rotate_y_bbox(hittables[hittableIndex].bbox, rotate), rotate, root);
  }

  uint32_t addConstantMedium(uint32_t boundary_hittable_index, precision_type density, uint32_t texture_index, bool root = true) {
    ConstantMedium constantMedium{
      .boundary_hittable_index = boundary_hittable_index,
      .neg_inv_density = -1 / density,
      .phase_function_material_index = addIsotropic(texture_index)
    };
    return addHittable(HittableType::ConstantMedium, hittables[boundary_hittable_index].bbox, constantMedium, root);
  }
  uint32_t addConstantMedium(
    uint32_t boundary_hittable_index, precision_type density, glm::vec<3, precision_type> albedo, bool root = true
  ) {
    ConstantMedium constantMedium{
      .boundary_hittable_index = boundary_hittable_index,
      .neg_inv_density = -1 / density,
      .phase_function_material_index = addIsotropic(albedo)
    };
    return addHittable(HittableType::ConstantMedium, hittables[boundary_hittable_index].bbox, constantMedium, root);
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

  uint32_t hittableCount() const { return static_cast<uint32_t>(hittables.size()); }

  uint32_t addBvhFromRange(uint32_t first, uint32_t count, bool root = true);

private:
  uint32_t addBoxNode(glm::vec<3, precision_type> a, glm::vec<3, precision_type> b, uint32_t materialIndex, bool root) {
    const uint32_t first = static_cast<uint32_t>(hittables.size());
    Aabb bbox;
    addBoxFaces(a, b, materialIndex, [&](uint32_t faceIndex) { bbox = Aabb(bbox, hittables[faceIndex].bbox); });
    HittableList list{.first = first, .count = static_cast<uint32_t>(hittables.size() - first)};
    return addHittable(HittableType::HittableList, bbox, list, root);
  }

  uint32_t addQuadNode(
    glm::vec<3, precision_type> Q, glm::vec<3, precision_type> u, glm::vec<3, precision_type> v, uint32_t materialIndex, bool root
  ) {
    const glm::vec<3, precision_type> n = glm::cross(u, v);
    const glm::vec<3, precision_type> normal = glm::normalize(n);
    Quad quad{
      .Q = Q,
      .u = u,
      .v = v,
      .w = n / glm::dot(n, n),
      .normal = normal,
      .D = glm::dot(normal, Q),
      .material_index = materialIndex,
    };
    return addHittable(HittableType::Quad, quad_bbox(Q, u, v), quad, root);
  }

  template <typename AddFace>
  void addBoxFaces(glm::vec<3, precision_type> a, glm::vec<3, precision_type> b, uint32_t materialIndex, AddFace addFace) {
    const glm::vec<3, precision_type> min = glm::min(a, b);
    const glm::vec<3, precision_type> max = glm::max(a, b);

    const glm::vec<3, precision_type> dx(max.x - min.x, 0, 0);
    const glm::vec<3, precision_type> dy(0, max.y - min.y, 0);
    const glm::vec<3, precision_type> dz(0, 0, max.z - min.z);

    addFace(addQuadNode({min.x, min.y, max.z}, dx, dy, materialIndex, false));
    addFace(addQuadNode({max.x, min.y, max.z}, -dz, dy, materialIndex, false));
    addFace(addQuadNode({max.x, min.y, min.z}, -dx, dy, materialIndex, false));
    addFace(addQuadNode({min.x, min.y, min.z}, dz, dy, materialIndex, false));
    addFace(addQuadNode({min.x, max.y, max.z}, dx, -dz, materialIndex, false));
    addFace(addQuadNode({min.x, min.y, min.z}, dx, dz, materialIndex, false));
  }

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

  template <typename Payload> uint32_t addHittable(HittableType type, const Aabb& bbox, const Payload& payload, bool root = true) {
    Hittable hittable{.type = type, .bbox = bbox};
    packPayload(hittable, payload);
    hittables.push_back(hittable);
    const uint32_t index = static_cast<uint32_t>(hittables.size() - 1);
    if (root) {
      rootHittables.push_back(index);
    }
    return index;
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

  static Aabb quad_bbox(glm::vec<3, precision_type> Q, glm::vec<3, precision_type> u, glm::vec<3, precision_type> v) {
    const Aabb bboxDiagonal1(Q, Q + u + v);
    const Aabb bboxDiagonal2(Q + u, Q + v);
    return Aabb(bboxDiagonal1, bboxDiagonal2);
  }

  static Aabb rotate_y_bbox(const Aabb& bbox, const RotateY& rotate) {
    glm::vec<3, precision_type> min(INF, INF, INF);
    glm::vec<3, precision_type> max(-INF, -INF, -INF);

    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 2; j++) {
        for (int k = 0; k < 2; k++) {
          const precision_type x = i * bbox.x.max + (1 - i) * bbox.x.min;
          const precision_type y = j * bbox.y.max + (1 - j) * bbox.y.min;
          const precision_type z = k * bbox.z.max + (1 - k) * bbox.z.min;

          const precision_type newX = rotate.cos_theta * x + rotate.sin_theta * z;
          const precision_type newZ = -rotate.sin_theta * x + rotate.cos_theta * z;
          const glm::vec<3, precision_type> tester(newX, y, newZ);

          min = glm::min(min, tester);
          max = glm::max(max, tester);
        }
      }
    }

    return {min, max};
  }
};

struct BvhBuilder {
  std::vector<Hittable>& hittables;
  std::vector<uint32_t>& rootHittables;

  explicit BvhBuilder(std::vector<Hittable>& hittables_, std::vector<uint32_t>& rootHittables_)
      : hittables(hittables_), rootHittables(rootHittables_) {}

  void build() {
    if (rootHittables.empty()) {
      return;
    }

    std::vector<uint32_t> hittableIndices(rootHittables.begin(), rootHittables.end());
    (void)build(hittableIndices, 0, hittableIndices.size(), true);
  }

  uint32_t build(std::vector<uint32_t>& hittableIndices, size_t start, size_t end, bool root) {
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
      if (root) {
        rootHittables.push_back(hittableIndices[start]);
      }
      return hittableIndices[start];
    }
    if (objectSpan == 2) {
      return addBvhNode(hittableIndices[start], hittableIndices[start + 1], spanBox, root);
    }

    const size_t mid = start + objectSpan / 2;
    const uint32_t left = build(hittableIndices, start, mid, false);
    const uint32_t right = build(hittableIndices, mid, end, false);
    return addBvhNode(left, right, spanBox, root);
  }

private:
  uint32_t addBvhNode(uint32_t left, uint32_t right, const Aabb& bbox, bool root = false) {
    BvhNode node(left, right);
    Hittable hittable{.type = HittableType::BvhNode, .bbox = bbox};
    std::memcpy(hittable.data.data(), &node, sizeof(node));
    hittables.push_back(hittable);
    const uint32_t index = static_cast<uint32_t>(hittables.size() - 1);
    if (root) {
      rootHittables.push_back(index);
    }
    return index;
  }
};

inline uint32_t RtCpuBuilder::addBvhFromRange(uint32_t first, uint32_t count, bool root) {
  std::vector<uint32_t> indices;
  indices.reserve(count);
  for (uint32_t i = 0; i < count; i++) {
    indices.push_back(first + i);
  }
  return BvhBuilder(hittables, rootHittables).build(indices, 0, indices.size(), root);
}
