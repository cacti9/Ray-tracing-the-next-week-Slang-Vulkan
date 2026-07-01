import rt;

struct UniformBuffer {
  precision_type4 pixel00_loc;
  precision_type4 pixel_delta_u;
  precision_type4 pixel_delta_v;
  precision_type4 camera_center;
  uint4 renderExtent;
  float deltaTime;
};
ConstantBuffer<UniformBuffer> ubo;

RWStructuredBuffer<uint> pixels;

color ray_color(inout uint state, ray r, uint maxDepth, const hittable world) {
  color throughput = color(1.0);

  for (uint depth = 0; depth < maxDepth; depth++) {
    hit_record rec;

    if (world.hit(r, { 0.001, INF }, rec)) {
      ray scattered;
      color attenuation;

      if (rec.mat.scatter(state, r, rec, attenuation, scattered)) {
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

ray get_ray(uint i, uint j, inout uint state) {
  vec3 offset = sample_square(state);
  point3 pixel_sample = ubo.pixel00_loc.rgb
                      + ((i + offset.x) * ubo.pixel_delta_u.rgb)
                      + ((j + offset.y) * ubo.pixel_delta_v.rgb);

  point3 ray_origin = ubo.camera_center.rgb;
  vec3 ray_direction = pixel_sample - ray_origin;

  return ray(ray_origin, ray_direction);
}

static constexpr uint MAGIC = 0x3F800000;
struct abc {
  uint zero;
  uint[2] a;
  __init() { zero = MAGIC; a = { 0 };}
}
[shader("compute")]
[numthreads(16, 16, 1)]
void compMain(uint3 threadId: SV_DispatchThreadID) {
  uint width = ubo.renderExtent.x;
  uint height = ubo.renderExtent.y;
  if (threadId.x >= width || threadId.y >= height) {
    return;
  }

  hittable_list<sphere, 4> world;

  Material material_ground = { MaterialType::Lambertian, reinterpret<uint[MATERIAL_DATA_SIZE]>(color( 0.8, 0.8, 0.0 )) };
  Material material_center = { MaterialType::Lambertian, reinterpret<uint[MATERIAL_DATA_SIZE]>(color(0.1, 0.2, 0.5)) };
  Material material_left = { MaterialType::Metal, reinterpret<uint[MATERIAL_DATA_SIZE]>( Metal(color( 0.8, 0.8, 0.8 ),0.3)) };
  Material material_right = { MaterialType::Metal, reinterpret<uint[MATERIAL_DATA_SIZE]>(Metal(color( 0.8, 0.6, 0.2 ),1.0)) };

  world.clear();
  world.add(sphere(point3(0, -100.5, -1), 100, material_ground));
  world.add(sphere(point3(0, 0, -1.2), 0.5, material_center));
  world.add(sphere(point3(-1.0, 0.0, -1), 0.5, material_left));
  world.add(sphere(point3( 1.0, 0.0, -1), 0.5, material_right));

  point3 camera_center = ubo.camera_center.rgb;
  vec3 pixel_delta_u = ubo.pixel_delta_u.rgb;
  vec3 pixel_delta_v = ubo.pixel_delta_v.rgb;
  point3 pixel00_loc = ubo.pixel00_loc.rgb;

  point3 pixel_center = pixel00_loc + (threadId.x * pixel_delta_u) + (threadId.y * pixel_delta_v);
  vec3 ray_direction = pixel_center - camera_center;
  ray r = { camera_center, ray_direction };

  color pixel_color = { 0. };
  uint state = threadId.y * width + threadId.x;
  for (int sample = 0; sample < SAMPLES_PER_PIXEL; ++sample) {
    ray r = get_ray(threadId.x, threadId.y, state);
    pixel_color += ray_color(state, r, MAX_DEPTH,world);
  }
  pixels[threadId.y * width + threadId.x] = precision_type3_to_packed_uint(pixel_color);
}
