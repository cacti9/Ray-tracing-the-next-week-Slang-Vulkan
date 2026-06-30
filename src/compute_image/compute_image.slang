import rt;

struct UniformBuffer {
  uint4 renderExtent;
  float deltaTime;
};
ConstantBuffer<UniformBuffer> ubo;

RWStructuredBuffer<uint> pixels;

color ray_color(const ray r) {
  vec3 unit_direction = normalize(r.dir);
  float a = 0.5 * (unit_direction.y + 1.0);
  return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
}

[shader("compute")]
[numthreads(16, 16, 1)]
void compMain(uint3 threadId: SV_DispatchThreadID) {
  uint width = ubo.renderExtent.x;
  uint height = ubo.renderExtent.y;
  if (threadId.x >= width || threadId.y >= height) {
    return;
  }

  float aspect_ratio = 1.0;
  float focal_length = 1.0;
  float viewport_height = 2.0;
  float viewport_width = viewport_height * (width / height);
  point3 camera_center = point3(0, 0, 0);

  // Calculate the vectors across the horizontal and down the vertical viewport edges.
  vec3 viewport_u = vec3(viewport_width, 0, 0);
  vec3 viewport_v = vec3(0, -viewport_height, 0);

  // Calculate the horizontal and vertical delta vectors from pixel to pixel.
  float3 pixel_delta_u = viewport_u / width;
  float3 pixel_delta_v = viewport_v / height;

  // Calculate the location of the upper left pixel.
  float3 viewport_upper_left = camera_center - vec3(0, 0, focal_length) - viewport_u/2 - viewport_v/2;
  float3 pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

  point3 pixel_center = pixel00_loc + (threadId.x * pixel_delta_u) + (threadId.y * pixel_delta_v);
  vec3 ray_direction = pixel_center - camera_center;
  ray r = { camera_center, ray_direction };

  color pixel_color = ray_color(r);
  // pixel_color.r = 255.999 * (float(threadId.x) / float(width - 1));
  // pixel_color.g = 255.999 * (float(threadId.y) / float(height - 1));
  // pixel_color.b = 0;
  pixels[threadId.y * width + threadId.x] = float3_to_packed_uint(pixel_color);
}
