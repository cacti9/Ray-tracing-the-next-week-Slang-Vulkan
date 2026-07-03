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
  BvhNode,
  Sphere,
};

public static const uint HITTABLE_DATA_SIZE = 16;
public struct Hittable {
  public HittableType type;
  public aabb bbox;
  public uint[HITTABLE_DATA_SIZE] data;
};

public struct Sphere : hittable {
  public ray center;
  public precision_type radius;
  public uint material_index;

  bool hit(const ray r, interval ray_t, inout hit_record rec) {
    point3 current_center = center.at(r.tm);
    vec3 oc = current_center - r.orig;
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
    vec3 outward_normal = (rec.p - current_center) / radius;
    rec.set_face_normal(r, outward_normal);
    // get_sphere_uv(outward_normal, rec.u, rec.v);
    rec.material_index = material_index;

    return true;
  }
};

public struct BvhNode : hittable {
  public uint left;
  public uint right;

  bool hit(const ray r, interval ray_t, inout hit_record rec) { return false; }
}

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
  uint stack[64];
  uint stack_size = 0;

  if (hittable_count == 0)
    return false;

  stack[stack_size++] = hittable_count - 1;

  while (stack_size > 0) {
    uint hittable_index = stack[--stack_size];
    bool hit_on_curr_hittable = false;
    switch (world[hittable_index].type) {
    case HittableType::Sphere:
      Sphere sphere = reinterpret<Sphere>(world[hittable_index].data);
      hit_on_curr_hittable = sphere.hit(r, interval(ray_t.min, closest_so_far), temp_rec);
      break;
    case HittableType::BvhNode:
      BvhNode node = reinterpret<BvhNode>(world[hittable_index].data);
      if (world[hittable_index].bbox.hit(r, interval(ray_t.min, closest_so_far))) {
        if (node.left == node.right) {
          if (stack_size < 64)
            stack[stack_size++] = node.left;
        } else {
          if (stack_size + 1 < 64) {
            stack[stack_size++] = node.left;
            stack[stack_size++] = node.right;
          }
        }
      }
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
