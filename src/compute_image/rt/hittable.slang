implementing rt;

public struct hit_record {
  public point3 p;
  public vec3 normal;
  public uint material_index;
  public precision_type t;
  public bool front_face;

  [mutating]
  internal void set_face_normal(const ray r, const vec3 outward_normal) {
    // Sets the hit record normal vector.
    // NOTE: the parameter `outward_normal` is assumed to have unit length.

    front_face = dot(r.dir, outward_normal) < 0;
    normal = front_face ? outward_normal : -outward_normal;
  }
}

internal interface hittable {
  bool hit(const ray r, interval ray_t, inout hit_record rec);
}

public enum HittableType {
  Sphere,
};

public static const uint HITTABLE_DATA_SIZE = 16;
public struct Hittable {
  public HittableType type;
  public uint[HITTABLE_DATA_SIZE] data;
};

public struct Sphere : hittable {
  public point3 center;
  public precision_type radius;
  public uint material_index;

  bool hit(const ray r, interval ray_t, inout hit_record rec) {
    vec3 oc = center - r.orig;
    precision_type a = dot(r.dir, r.dir);
    precision_type h = dot(r.dir, oc);
    precision_type c = dot(oc, oc) - radius*radius;

    precision_type discriminant = h*h - a*c;
    if (discriminant < 0)
      return false;

    precision_type sqrtd = sqrt(discriminant);

    // Find the nearest root that lies in the acceptable range.
    precision_type root = (h - sqrtd) / a;
    if (!ray_t.surrounds(root)) {
      root = (h + sqrtd) / a;
      if (!ray_t.surrounds(root)) 
        return false;
    }

    rec.t = root;
    rec.p = r.at(rec.t);
    rec.normal = (rec.p - center) / radius;
    vec3 outward_normal = (rec.p - center) / radius;
    rec.set_face_normal(r, outward_normal);
    rec.material_index = material_index;

    return true;
  }
};

// // role of shared pointer<type>
// public struct hittableRef {
//   uint type;
//   uint index; // index in SSBO
// };

public bool hit(
  StructuredBuffer<Hittable> world,
  uint hittable_count,
  ray r,
  interval ray_t,
  inout hit_record rec) {
  hit_record temp_rec;
  bool hit_anything = false;
  precision_type closest_so_far = ray_t.max;

  for (uint i = 0; i < hittable_count; i++) {
    bool hit_on_curr_hittable = false;
    switch (world[i].type) {
    case HittableType::Sphere:
      Sphere sphere = reinterpret<Sphere>(world[i].data);
      hit_on_curr_hittable = sphere.hit(r, {ray_t.min, closest_so_far}, temp_rec);
      break;
    default:
      continue;
    }
    if (hit_on_curr_hittable) {
      hit_anything = true;
      closest_so_far = temp_rec.t;
      rec = temp_rec;
    }
  }

  return hit_anything;
}
