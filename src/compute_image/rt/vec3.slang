implementing rt;

public typealias vec3 = precision_type3;
public typealias point3 = precision_type3;
public typealias color = precision_type3;

uint wangHash(uint seed) {
  seed = (seed ^ 61u) ^ (seed >> 16);
  seed *= 9u;
  seed = seed ^ (seed >> 4);
  seed *= 0x27d4eb2du;
  seed = seed ^ (seed >> 15);
  return seed;
}
public precision_type random(inout uint state) {
  state = wangHash(state);
  return state / 4294967296.0;
}
public precision_type random(inout uint state, precision_type min, precision_type max) {
  return min + (max - min) * random(state);
}
public vec3 sample_square(inout uint state) {
  // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
  return vec3(random(state) - 0.5, random(state) - 0.5, 0);
}
public vec3 random3(inout uint state) {
  return vec3(random(state), random(state), random(state));
}
public vec3 random3(inout uint state, precision_type min, precision_type max) {
  return vec3(random(state,min,max), random(state,min,max), random(state,min,max));
}
public vec3 random_unit_vector(inout uint state) {
  while (true) {
    vec3 p = random3(state, -1,1);
    precision_type lensq = dot(p,p);
    if (1e-8 < lensq && lensq <= 1)
      return p / sqrt(lensq);
  }
}
public vec3 random_on_hemisphere(inout uint state, const vec3 normal) {
  vec3 on_unit_sphere = random_unit_vector(state);
  if (dot(on_unit_sphere, normal) > 0.0) // In the same hemisphere as the normal
    return on_unit_sphere;
  else
    return -on_unit_sphere;
}
public bool nearZero(precision_type3 v) {
    const precision_type s = 1e-8;
    return all(abs(v) < s);
}

public vec3 reflect(const vec3 v, const vec3 n) {
  return v - 2*dot(v,n)*n;
}

public vec3 refract(const vec3 uv, const vec3 n, double etai_over_etat) {
  precision_type cos_theta = min(dot(-uv, n), 1.0);
  vec3 r_out_perp =  etai_over_etat * (uv + cos_theta*n);
  vec3 r_out_parallel = -sqrt(abs(1.0 - dot(r_out_perp,r_out_perp))) * n;
  return r_out_perp + r_out_parallel;
}