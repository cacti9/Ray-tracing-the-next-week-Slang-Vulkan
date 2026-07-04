#include "compute_image_renderer.h"

#include <random>

namespace {
void bouncing_spheres(RtCpuBuilder& builder, CameraSettings& camera) {
  camera = {
    .look_from = {13, 2, 3},
    .look_at = {0, 0, 0},
    .vup = {0, 1, 0},
    .vfov = 20.0,
    .defocus_angle = 0.6,
    .focus_dist = 10.0,
  };

  std::mt19937 randomEngine{1};
  std::uniform_real_distribution<precision_type> unitDistribution{0.0, 1.0};
  auto random = [&]() { return unitDistribution(randomEngine); };
  auto randomRange = [&](precision_type min, precision_type max) { return min + (max - min) * random(); };
  auto randomColor = [&]() { return glm::vec<3, precision_type>{random(), random(), random()}; };
  auto randomColorRange = [&](precision_type min, precision_type max) {
    return glm::vec<3, precision_type>{
      randomRange(min, max),
      randomRange(min, max),
      randomRange(min, max),
    };
  };

  const uint32_t checker = builder.addCheckerTexture(0.32, {0.2, 0.3, 0.1}, {0.9, 0.9, 0.9});
  const uint32_t groundMaterial = builder.addLambertian(checker);
  builder.addStaticSphere({0, -1000, 0}, 1000, groundMaterial);

  for (int a = -11; a < 11; a++) {
    for (int b = -11; b < 11; b++) {
      const precision_type chooseMat = random();
      const glm::vec<3, precision_type> center{
        a + 0.9 * random(),
        0.2,
        b + 0.9 * random(),
      };

      if (glm::length(center - glm::vec<3, precision_type>{4, 0.2, 0}) > 0.9) {
        if (chooseMat < 0.8) {
          const auto albedo = randomColor() * randomColor();
          auto center2 = center + glm::vec<3, precision_type>{0, randomRange(0, .5), 0};
          builder.addMovingSphere(center, center2, 0.2, builder.addLambertian(albedo));
        } else if (chooseMat < 0.95) {
          const auto albedo = randomColorRange(0.5, 1.0);
          const precision_type fuzz = randomRange(0, 0.5);
          builder.addStaticSphere(center, 0.2, builder.addMetal(albedo, fuzz));
        } else {
          builder.addStaticSphere(center, 0.2, builder.addDielectric(1.5));
        }
      }
    }
  }

  builder.addStaticSphere({0, 1, 0}, 1.0, builder.addDielectric(1.5));
  builder.addStaticSphere({-4, 1, 0}, 1.0, builder.addLambertian({0.4, 0.2, 0.1}));
  builder.addStaticSphere({4, 1, 0}, 1.0, builder.addMetal({0.7, 0.6, 0.5}, 0.0));
}

void checkered_spheres(RtCpuBuilder& builder, CameraSettings& camera) {
  camera = {
    .look_from = {13, 2, 3},
    .look_at = {0, 0, 0},
    .vup = {0, 1, 0},
    .vfov = 20.0,
    .defocus_angle = 0.0,
    .focus_dist = 10.0,
  };

  const uint32_t checker = builder.addCheckerTexture(0.32, {0.2, 0.3, 0.1}, {0.9, 0.9, 0.9});
  const uint32_t checkerMaterial = builder.addLambertian(checker);

  builder.addStaticSphere({0, -10, 0}, 10, checkerMaterial);
  builder.addStaticSphere({0, 10, 0}, 10, checkerMaterial);
}

void earth(RtCpuBuilder& builder, CameraSettings& camera) {
  camera = {
    .look_from = {0, 0, 12},
    .look_at = {0, 0, 0},
    .vup = {0, 1, 0},
    .vfov = 20.0,
    .defocus_angle = 0.0,
    .focus_dist = 10.0,
  };

  const uint32_t earthTexture = builder.addImageTexture("earthmap.jpg");
  const uint32_t earthSurface = builder.addLambertian(earthTexture);
  builder.addStaticSphere({0, 0, 0}, 2, earthSurface);
}

void perlin_spheres(RtCpuBuilder& builder, CameraSettings& camera) {
  camera = {
    .look_from = {13, 2, 3},
    .look_at = {0, 0, 0},
    .vup = {0, 1, 0},
    .vfov = 20.0,
    .defocus_angle = 0.0,
    .focus_dist = 10.0,
  };

  const uint32_t perlinTexture = builder.addNoiseTexture(4);
  const uint32_t perlinMaterial = builder.addLambertian(perlinTexture);

  builder.addStaticSphere({0, -1000, 0}, 1000, perlinMaterial);
  builder.addStaticSphere({0, 2, 0}, 2, perlinMaterial);
}

void quads(RtCpuBuilder& builder, CameraSettings& camera) {
  camera = {
    .look_from = {0, 0, 9},
    .look_at = {0, 0, 0},
    .vup = {0, 1, 0},
    .vfov = 80.0,
    .defocus_angle = 0.0,
    .focus_dist = 10.0,
  };

  const uint32_t leftRed = builder.addLambertian({1.0, 0.2, 0.2});
  const uint32_t backGreen = builder.addLambertian({0.2, 1.0, 0.2});
  const uint32_t rightBlue = builder.addLambertian({0.2, 0.2, 1.0});
  const uint32_t upperOrange = builder.addLambertian({1.0, 0.5, 0.0});
  const uint32_t lowerTeal = builder.addLambertian({0.2, 0.8, 0.8});

  builder.addQuad({-3, -2, 5}, {0, 0, -4}, {0, 4, 0}, leftRed);
  builder.addQuad({-2, -2, 0}, {4, 0, 0}, {0, 4, 0}, backGreen);
  builder.addQuad({3, -2, 1}, {0, 0, 4}, {0, 4, 0}, rightBlue);
  builder.addQuad({-2, 3, 1}, {4, 0, 0}, {0, 0, 4}, upperOrange);
  builder.addQuad({-2, -3, 5}, {4, 0, 0}, {0, 0, -4}, lowerTeal);
}

void simple_light(RtCpuBuilder& builder, CameraSettings& camera) {
  camera = {
    .look_from = {26, 3, 6},
    .look_at = {0, 2, 0},
    .vup = {0, 1, 0},
    .background = {0, 0, 0},
    .vfov = 20.0,
    .defocus_angle = 0.0,
    .focus_dist = 10.0,
  };

  const uint32_t perlinTexture = builder.addNoiseTexture(4);
  const uint32_t perlinMaterial = builder.addLambertian(perlinTexture);

  builder.addStaticSphere({0, -1000, 0}, 1000, perlinMaterial);
  builder.addStaticSphere({0, 2, 0}, 2, perlinMaterial);

  const uint32_t difflight = builder.addDiffuseLight({4, 4, 4});
  builder.addStaticSphere({0, 7, 0}, 2, difflight);
  builder.addQuad({3, 1, -2}, {2, 0, 0}, {0, 2, 0}, difflight);
}

void cornell_box(RtCpuBuilder& builder, CameraSettings& camera) {
  camera = {
    .look_from = {278, 278, -800},
    .look_at = {278, 278, 0},
    .vup = {0, 1, 0},
    .background = {0, 0, 0},
    .vfov = 40.0,
    .defocus_angle = 0.0,
    .focus_dist = 10.0,
  };

  const uint32_t red = builder.addLambertian({0.65, 0.05, 0.05});
  const uint32_t white = builder.addLambertian({0.73, 0.73, 0.73});
  const uint32_t green = builder.addLambertian({0.12, 0.45, 0.15});
  const uint32_t light = builder.addDiffuseLight({15, 15, 15});

  builder.addQuad({555, 0, 0}, {0, 555, 0}, {0, 0, 555}, green);
  builder.addQuad({0, 0, 0}, {0, 555, 0}, {0, 0, 555}, red);
  builder.addQuad({343, 554, 332}, {-130, 0, 0}, {0, 0, -105}, light);
  builder.addQuad({0, 0, 0}, {555, 0, 0}, {0, 0, 555}, white);
  builder.addQuad({555, 555, 555}, {-555, 0, 0}, {0, 0, -555}, white);
  builder.addQuad({0, 0, 555}, {555, 0, 0}, {0, 555, 0}, white);
}
} // namespace

void ComputeImageRenderer::populateWorld() {
  hittablesData.clear();
  materialsData.clear();
  texturesData.clear();
  perlinsData.clear();
  imageTexturePaths.clear();

  RtCpuBuilder builder(hittablesData, materialsData, texturesData, imageTexturePaths, perlinsData);

  switch (7) {
  case 1:
    bouncing_spheres(builder, cameraSettings);
    break;
  case 2:
    checkered_spheres(builder, cameraSettings);
    break;
  case 3:
    earth(builder, cameraSettings);
    break;
  case 4:
    perlin_spheres(builder, cameraSettings);
    break;
  case 5:
    quads(builder, cameraSettings);
    break;
  case 6:
    simple_light(builder, cameraSettings);
    break;
  case 7:
    cornell_box(builder, cameraSettings);
    break;
  }

  BvhBuilder(hittablesData).build();

  if (perlinsData.empty())
    perlinsData.emplace_back();

  hittableBufferSize = sizeof(Hittable) * hittablesData.size();
  materialBufferSize = sizeof(Material) * materialsData.size();
  textureBufferSize = sizeof(Texture) * texturesData.size();
  perlinBufferSize = sizeof(Perlin) * perlinsData.size();
}
