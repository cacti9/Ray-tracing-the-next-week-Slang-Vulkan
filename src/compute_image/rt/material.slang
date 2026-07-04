implementing rt;

internal interface material {
  bool scatter(
    TextureContext texture_context,
    inout uint rngState,
    const ray r_in,
    const hit_record rec,
    out color attenuation,
    inout ray scattered
  );
}

public enum MaterialType {
  Lambertian,
  Metal,
  Dielectric,
};

public static const uint MATERIAL_DATA_SIZE = RT_PRECISION_IS_DOUBLE ? 8 : 4;
public struct Material : material {
  public MaterialType type; // 32bit
  public uint[MATERIAL_DATA_SIZE] data;

  public bool scatter(
    TextureContext texture_context,
    inout uint rngState,
    const ray r_in,
    const hit_record rec,
    out color attenuation,
    inout ray scattered
  ) {
    switch (type) {
    case MaterialType::Lambertian:
      Lambertian l= reinterpret<Lambertian>(data);
      return l.scatter(texture_context, rngState, r_in, rec, attenuation, scattered);
    case MaterialType::Metal:
      Metal m = reinterpret<Metal>(data);
      return m.scatter(texture_context, rngState, r_in, rec, attenuation, scattered);
    case MaterialType::Dielectric:
      Dielectric d = reinterpret<Dielectric>(data);
      return d.scatter(texture_context, rngState, r_in, rec, attenuation, scattered);
    default:
      attenuation = {0.}; // idk
      return false; // idk
    }
  }
}

public struct Lambertian : material {
  public uint texture_index;

  bool scatter(
    TextureContext texture_context,
    inout uint rngState,
    const ray r_in,
    const hit_record rec,
    out color attenuation,
    inout ray scattered
  ) {
    vec3 scatter_direction;
    scatter_direction = rec.normal + random_unit_vector(rngState);

    if (nearZero(scatter_direction))
      scatter_direction = rec.normal;

    scattered = ray(rec.p, scatter_direction, r_in.tm);
    attenuation = texture_value(texture_context, texture_index, rec.u, rec.v, rec.p);
    return true;
  }
}

public struct Metal : material {
  public color albedo;
  public precision_type fuzz;

  bool scatter(
    TextureContext texture_context,
    inout uint rngState,
    const ray r_in,
    const hit_record rec,
    out color attenuation,
    inout ray scattered
  ) {
    vec3 reflected = reflect(r_in.dir, rec.normal);
    reflected = normalize(reflected) + (fuzz * random_unit_vector(rngState));
    scattered = ray(rec.p, reflected, r_in.tm);
    attenuation = albedo;
    return (dot(scattered.dir, rec.normal) > 0);
  }
}

public struct Dielectric : material {
  public precision_type refraction_index;

  bool scatter(
    TextureContext texture_context,
    inout uint rngState,
    const ray r_in,
    const hit_record rec,
    out color attenuation,
    inout ray scattered
  ) {
    attenuation = color(1.0, 1.0, 1.0);
    precision_type ri = rec.front_face ? (1.0/refraction_index) : refraction_index;

    vec3 unit_direction = normalize(r_in.dir);
    precision_type cos_theta = min(dot(-unit_direction, rec.normal), 1.0);
    precision_type sin_theta = sqrt(1.0 - cos_theta*cos_theta);

    bool cannot_refract = ri * sin_theta > 1.0;
    vec3 direction;

    if (cannot_refract || reflectance(cos_theta, ri) > random(rngState))
      direction = reflect(unit_direction, rec.normal);
    else
      direction = refract(unit_direction, rec.normal, ri);

    scattered = ray(rec.p, direction, r_in.tm);
    return true;
  }

  static precision_type reflectance(precision_type cosine, precision_type refraction_index) {
    // Use Schlick's approximation for reflectance.
    precision_type r0 = (1 - refraction_index) / (1 + refraction_index);
    r0 = r0*r0;
    precision_type x = 1 - cosine;
    precision_type x2 = x * x;
    return r0 + (1-r0) * x2 * x2 * x;
  }
}
