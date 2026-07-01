implementing rt;

public enum MaterialType {
  Lambertian,
  Metal,
  Dielectric,
};

public static const uint MATERIAL_DATA_SIZE = 8;
public struct Material {
  public MaterialType type; // 32bit
  public uint[MATERIAL_DATA_SIZE] data;

  public bool scatter(inout uint state, const ray r_in, const hit_record rec, out color attenuation, inout ray scattered) {
    vec3 scatter_direction;
    switch (type) {
    case MaterialType::Lambertian:
      Lambertian l= reinterpret<Lambertian>(data);
      scatter_direction = rec.normal + random_unit_vector(state);

      if (nearZero(scatter_direction))
        scatter_direction = rec.normal;

      scattered = ray(rec.p, scatter_direction);
      attenuation = l.albedo;
      return true;
    case MaterialType::Metal:
      Metal m = reinterpret<Metal>(data);
      vec3 reflected = reflect(r_in.dir, rec.normal);
      reflected = normalize(reflected) + (m.fuzz * random_unit_vector(state));
      scattered = ray(rec.p, reflected);
      attenuation = m.albedo;
      return (dot(scattered.dir, rec.normal) > 0);
    case MaterialType::Dielectric:
      Dielectric d = reinterpret<Dielectric>(data);
      attenuation = color(1.0, 1.0, 1.0);
      precision_type ri = rec.front_face ? (1.0/d.refraction_index) : d.refraction_index;

      vec3 unit_direction = normalize(r_in.dir);
      precision_type cos_theta = min(dot(-unit_direction, rec.normal), 1.0);
      precision_type sin_theta = sqrt(1.0 - cos_theta*cos_theta);

      bool cannot_refract = ri * sin_theta > 1.0;
      vec3 direction;

      if (cannot_refract || d::reflectance(cos_theta, ri) > random(state))
          direction = reflect(unit_direction, rec.normal);
      else
          direction = refract(unit_direction, rec.normal, ri);

      scattered = ray(rec.p, direction);
      return true;
    default:
      attenuation = {0.}; // idk
      return false; // idk
    }
  }
}

public struct Lambertian {
  public color albedo;
}

public struct Metal {
  public color albedo;
  public precision_type fuzz;
}

public struct Dielectric {
  public precision_type refraction_index;

  public static precision_type reflectance(precision_type cosine, precision_type refraction_index) {
    // Use Schlick's approximation for reflectance.
    precision_type r0 = (1 - refraction_index) / (1 + refraction_index);
    r0 = r0*r0;
    return r0 + (1-r0)*pow((1 - cosine),5);
  }
}