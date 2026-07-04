implementing rt;

public struct hit_record {
  public point3 p;
  public vec3 normal;
  public uint material_index;
  public precision_type t;
  public precision_type u;
  public precision_type v;
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
  Quad,
};

public static const uint HITTABLE_DATA_SIZE = RT_PRECISION_IS_DOUBLE ? 40 : 24;
public struct Hittable {
  public HittableType type;
  public aabb bbox;
  public uint[HITTABLE_DATA_SIZE] data;
};

public void get_sphere_uv(const point3 p, out precision_type u, out precision_type v) {
  float theta = acos(float(-p.y));
  float phi = atan2(float(-p.z), float(p.x)) + float(PI);

  u = precision_type(phi / (2.0f * float(PI)));
  v = precision_type(theta / float(PI));
}

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
    get_sphere_uv(outward_normal, rec.u, rec.v);
    rec.set_face_normal(r, outward_normal);
    rec.material_index = material_index;

    return true;
  }
};

public struct BvhNode : hittable {
  public uint left;
  public uint right;

  bool hit(const ray r, interval ray_t, inout hit_record rec) { return false; }
}

public struct Quad : hittable {
  public point3 Q;
  public vec3 u;
  public vec3 v;
  public vec3 w;
  public vec3 normal;
  public precision_type D;
  public uint material_index;

  bool hit(const ray r, interval ray_t, inout hit_record rec) {
    precision_type denom = dot(normal, r.dir);

    if (abs(denom) < 1e-8)
      return false;

    precision_type t = (D - dot(normal, r.orig)) / denom;
    if (!ray_t.contains(t))
      return false;

    point3 intersection = r.at(t);
    vec3 planar_hitpt_vector = intersection - Q;
    precision_type alpha = dot(w, cross(planar_hitpt_vector, v));
    precision_type beta = dot(w, cross(u, planar_hitpt_vector));

    if (!is_interior(alpha, beta, rec))
      return false;

    rec.t = t;
    rec.p = intersection;
    rec.set_face_normal(r, normal);
    rec.material_index = material_index;

    return true;
  }

  bool is_interior(precision_type a, precision_type b, inout hit_record rec) {
    interval unit_interval = interval(0, 1);

    if (!unit_interval.contains(a) || !unit_interval.contains(b))
      return false;

    rec.u = a;
    rec.v = b;
    return true;
  }
}

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
    case HittableType::Quad:
      Quad quad = reinterpret<Quad>(world[hittable_index].data);
      hit_on_curr_hittable = quad.hit(r, interval(ray_t.min, closest_so_far), temp_rec);
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
