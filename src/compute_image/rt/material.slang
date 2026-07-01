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