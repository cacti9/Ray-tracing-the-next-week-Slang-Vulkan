module rt;

public typealias precision_type = float;
public typealias precision_type3 = vector<precision_type,3>;
public typealias precision_type4 = vector<precision_type,4>;

public static constexpr precision_type PI = 3.1415926535897932385;
public static constexpr precision_type INF = 1e30;

public precision_type degrees_to_radians(precision_type degrees) {
    return degrees * PI / 180.0;
}

__include "rt/vec3.slang";
__include "rt/ray.slang";
__include "rt/interval.slang";
__include "rt/aabb.slang";
__include "rt/hittable.slang";
__include "rt/texture.slang";
__include "rt/material.slang";


public uint precision_type3_to_packed_uint(precision_type3 x, precision_type samples_per_pixel) {
  x /= samples_per_pixel;
  x.r = linear_to_gamma(x.r);
  x.g = linear_to_gamma(x.g);
  x.b = linear_to_gamma(x.b);
  x = clamp(x, 0.0, 0.999);
  return uint(255.999*x.b) | (uint(255.999*x.g) << 8) | (uint(255.999*x.r) << 16) | (255u << 24);
}

public precision_type linear_to_gamma(precision_type linear_component) {
  if (linear_component > 0.)
    return sqrt(linear_component);
  return 0.;
}
