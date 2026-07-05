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
  builder.addBox({0, 0, 0}, {165, 330, 165}, 15, {265, 0, 295}, white);
  builder.addBox({0, 0, 0}, {165, 165, 165}, -18, {130, 0, 65}, white);
}

void cornell_smoke(RtCpuBuilder& builder, CameraSettings& camera) {
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
  const uint32_t light = builder.addDiffuseLight({7, 7, 7});

  builder.addQuad({555, 0, 0}, {0, 555, 0}, {0, 0, 555}, green);
  builder.addQuad({0, 0, 0}, {0, 555, 0}, {0, 0, 555}, red);
  builder.addQuad({113, 554, 127}, {330, 0, 0}, {0, 0, 305}, light);
  builder.addQuad({0, 0, 0}, {555, 0, 0}, {0, 0, 555}, white);
  builder.addQuad({555, 555, 555}, {-555, 0, 0}, {0, 0, -555}, white);
  builder.addQuad({0, 0, 555}, {555, 0, 0}, {0, 555, 0}, white);

  uint32_t box1 = builder.addBox({0, 0, 0}, {165, 330, 165}, 15, {265, 0, 295}, white, false);
  uint32_t box2 = builder.addBox({0, 0, 0}, {165, 165, 165}, -18, {130, 0, 65}, white, false);

  builder.addConstantMedium(box1, 0.01, {0, 0, 0});
  builder.addConstantMedium(box2, 0.01, {1, 1, 1});
}

void final_scene(RtCpuBuilder& builder, CameraSettings& camera) {
  camera = {
    .look_from = {478, 278, -600},
    .look_at = {278, 278, 0},
    .vup = {0, 1, 0},
    .background = {0, 0, 0},
    .vfov = 40.0,
    .defocus_angle = 0.0,
    .focus_dist = 10.0,
  };

  std::mt19937 randomEngine{1};
  auto randomRange = [&](precision_type min, precision_type max) {
    std::uniform_real_distribution<precision_type> distribution{min, max};
    return distribution(randomEngine);
  };

  const uint32_t ground = builder.addLambertian({0.48, 0.83, 0.53});
  const uint32_t boxes1First = builder.hittableCount();
  constexpr int boxesPerSide = 20;
  for (int i = 0; i < boxesPerSide; i++) {
    for (int j = 0; j < boxesPerSide; j++) {
      const precision_type w = 100.0;
      const precision_type x0 = -1000.0 + i * w;
      const precision_type z0 = -1000.0 + j * w;
      const precision_type y0 = 0.0;
      const precision_type x1 = x0 + w;
      const precision_type y1 = randomRange(1, 101);
      const precision_type z1 = z0 + w;

      builder.addBox({x0, y0, z0}, {x1, y1, z1}, ground, false);
    }
  }
  builder.addBvhFromRange(boxes1First, builder.hittableCount() - boxes1First);

  const uint32_t light = builder.addDiffuseLight({7, 7, 7});
  builder.addQuad({123, 554, 147}, {300, 0, 0}, {0, 0, 265}, light);

  const glm::vec<3, precision_type> center1{400, 400, 200};
  const glm::vec<3, precision_type> center2 = center1 + glm::vec<3, precision_type>{30, 0, 0};
  builder.addMovingSphere(center1, center2, 50, builder.addLambertian({0.7, 0.3, 0.1}));
  builder.addStaticSphere({260, 150, 45}, 50, builder.addDielectric(1.5));
  builder.addStaticSphere({0, 150, 145}, 50, builder.addMetal({0.8, 0.8, 0.9}, 1.0));

  const uint32_t glassBoundary = builder.addStaticSphere({360, 150, 145}, 70, builder.addDielectric(1.5));
  builder.addConstantMedium(glassBoundary, 0.2, {0.2, 0.4, 0.9});
  const uint32_t atmosphereBoundary = builder.addStaticSphere({0, 0, 0}, 5000, builder.addDielectric(1.5), false);
  builder.addConstantMedium(atmosphereBoundary, 0.0001, {1, 1, 1});

  builder.addStaticSphere({400, 200, 400}, 100, builder.addLambertian(builder.addImageTexture("earthmap.jpg")));
  builder.addStaticSphere({220, 280, 300}, 80, builder.addLambertian(builder.addNoiseTexture(0.2)));

  const uint32_t white = builder.addLambertian({0.73, 0.73, 0.73});
  const uint32_t boxes2First = builder.hittableCount();
  constexpr int sphereCount = 1000;
  for (int j = 0; j < sphereCount; j++) {
    builder.addStaticSphere({randomRange(0, 165), randomRange(0, 165), randomRange(0, 165)}, 10, white, false);
  }
  const uint32_t boxes2Bvh = builder.addBvhFromRange(boxes2First, builder.hittableCount() - boxes2First, false);
  const uint32_t rotatedBoxes2 = builder.addRotateY(boxes2Bvh, 15, false);
  builder.addTranslate(rotatedBoxes2, {-100, 270, 395});
}
} // namespace

void ComputeImageRenderer::populateWorld() {
  hittablesData.clear();
  materialsData.clear();
  texturesData.clear();
  perlinsData.clear();
  imageTexturePaths.clear();
  std::vector<uint32_t> rootHittables;

  RtCpuBuilder builder(hittablesData, materialsData, texturesData, imageTexturePaths, perlinsData, rootHittables);

  switch (9) {
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
  case 8:
    cornell_smoke(builder, cameraSettings);
    break;
  case 9:
    final_scene(builder, cameraSettings);
    break;
  }

  BvhBuilder(hittablesData, rootHittables).build();

  if (perlinsData.empty())
    perlinsData.emplace_back();

  hittableBufferSize = sizeof(Hittable) * hittablesData.size();
  materialBufferSize = sizeof(Material) * materialsData.size();
  textureBufferSize = sizeof(Texture) * texturesData.size();
  perlinBufferSize = sizeof(Perlin) * perlinsData.size();
}
