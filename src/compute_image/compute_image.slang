import rt;

struct UniformBuffer {
  precision_type4 pixel00_loc;
  precision_type4 pixel_delta_u;
  precision_type4 pixel_delta_v;
  precision_type4 camera_center;
  precision_type4 defocus_disk_u;
  precision_type4 defocus_disk_v;
  uint2 renderExtent;
  uint hittable_count;
  uint max_depth;
  uint samples_per_pixel;
  float defocus_angle;
};
ConstantBuffer<UniformBuffer> ubo;

RWStructuredBuffer<uint> pixels;
StructuredBuffer<Hittable> the_world;
StructuredBuffer<Material> the_world_mat;

color ray_color(inout uint rngState, ray r, uint maxDepth) {
  color throughput = color(1.0);

  hit_record rec;
  ray scattered;
  color attenuation;
  for (uint depth = 0; depth < maxDepth; depth++) {

    if (hit(the_world, ubo.hittable_count, r, { 0.001, INF }, rec)) {
      if (the_world_mat[rec.material_index].scatter(rngState, r, rec, attenuation, scattered)) {
        throughput *= attenuation;
        r = scattered;
      } else {
        return color(0.0);
      }
    } else {
      vec3 unit_direction = normalize(r.dir);
      precision_type a = 0.5 * (unit_direction.y + 1.0);
      color sky = (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);

      return throughput * sky;
    }
  }

  return color(0.0);
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
