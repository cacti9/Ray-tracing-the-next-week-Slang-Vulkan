import rt;

struct UniformBuffer {
  precision_type4 pixel00_loc;
  precision_type4 pixel_delta_u;
  precision_type4 pixel_delta_v;
  precision_type4 camera_center;
  precision_type4 defocus_disk_u;
  precision_type4 defocus_disk_v;
  precision_type4 background;
  uint2 renderExtent;
  uint hittable_count;
  uint max_depth;
  uint samples_per_pixel;
  float defocus_angle;
};
[[vk::binding(0, 0)]] ConstantBuffer<UniformBuffer> ubo;

[[vk::binding(1, 0)]] RWStructuredBuffer<uint> pixels;
[[vk::binding(2, 0)]] StructuredBuffer<Hittable> the_world;
[[vk::binding(3, 0)]] StructuredBuffer<Material> the_world_mat;
[[vk::binding(4, 0)]] StructuredBuffer<Texture> the_textures;
[[vk::binding(5, 0)]] Texture2D<float4> image_textures[MAX_IMAGE_TEXTURES];
[[vk::binding(6, 0)]] SamplerState image_sampler;
[[vk::binding(7, 0)]] StructuredBuffer<Perlin> the_perlins;

color ray_color(inout uint rngState, ray r, uint maxDepth) {
  TextureContext texture_context = { the_textures, image_textures, image_sampler, the_perlins };
  color path_weight = color(1.0);
  color accumulated = color(0.0);

  hit_record rec;
  ray scattered;
  color attenuation;
  for (uint depth = 0; depth < maxDepth; depth++) {

    if (hit(the_world, ubo.hittable_count, rngState, r, interval( 0.001, INF ), rec)) {
      Material material = the_world_mat[rec.material_index];
      color from_emission = path_weight * material.emitted(texture_context, rec.u, rec.v, rec.p);
      accumulated += from_emission;

      if (material.scatter(texture_context, rngState, r, rec, attenuation, scattered)) { // 1. hit an object with reflective material
        path_weight *= attenuation;
        r = scattered;
      } else { // 2. hit an object, but does not reflect. mostly light emitting material
        return accumulated;
      }
    } else { // 3. hit the background
      color from_background = path_weight * ubo.background.rgb;
      return accumulated + from_background;
    }
  }

  return accumulated;
}

point3 defocus_disk_sample(inout uint rngState) {
  point3 p = random_in_unit_disk(rngState);
  return ubo.camera_center.rgb + (p.r * ubo.defocus_disk_u.rgb) + (p[1] * ubo.defocus_disk_v.rgb);
}
ray get_ray(inout uint rngState, uint i, uint j) {
  vec3 offset = sample_square(rngState);
  point3 pixel_sample = ubo.pixel00_loc.rgb
                      + ((i + offset.x) * ubo.pixel_delta_u.rgb)
                      + ((j + offset.y) * ubo.pixel_delta_v.rgb);

  point3 ray_origin = (ubo.defocus_angle <= 0.) ? ubo.camera_center.rgb : defocus_disk_sample(rngState);
  vec3 ray_direction = pixel_sample - ray_origin;
  precision_type ray_time = random(rngState);

  return ray(ray_origin, ray_direction, ray_time);
}

[shader("compute")]
[numthreads(16, 16, 1)]
void compMain(uint3 threadId: SV_DispatchThreadID) {
  uint width = ubo.renderExtent.x;
  uint height = ubo.renderExtent.y;
  uint max_depth = ubo.max_depth;
  precision_type samples_per_pixel = ubo.samples_per_pixel;
  if (threadId.x >= width || threadId.y >= height) {
    return;
  }

  point3 camera_center = ubo.camera_center.rgb;
  vec3 pixel_delta_u = ubo.pixel_delta_u.rgb;
  vec3 pixel_delta_v = ubo.pixel_delta_v.rgb;
  point3 pixel00_loc = ubo.pixel00_loc.rgb;

  point3 pixel_center = pixel00_loc + (threadId.x * pixel_delta_u) + (threadId.y * pixel_delta_v);
  vec3 ray_direction = pixel_center - camera_center;
  ray r = { camera_center, ray_direction };

  color pixel_color = { 0. };
  uint rngState = threadId.y * width + threadId.x;
  for (int sample = 0; sample < samples_per_pixel; ++sample) {
    ray r = get_ray(rngState, threadId.x, threadId.y);
    pixel_color += ray_color(rngState, r, max_depth);
  }
  pixels[threadId.y * width + threadId.x] = precision_type3_to_packed_uint(pixel_color, samples_per_pixel);
}
