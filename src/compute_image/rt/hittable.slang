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
    //the parameter `outward_normal` is assumed to have unit length.

    front_face = dot(r.dir, outward_normal) < 0;
    normal = front_face ? outward_normal : -outward_normal;
  }
}

public enum HittableType {
  BvhNode,
  Sphere,
  Quad,
  HittableList,
  Translate,
  RotateY,
};

public struct BvhNode {
  public uint left;
  public uint right;
}

public static const uint HITTABLE_DATA_SIZE = RT_PRECISION_IS_DOUBLE ? 40 : 24;
public struct Hittable {
  public HittableType type;
  public aabb bbox;
  public uint[HITTABLE_DATA_SIZE] data;
};

public struct Sphere {
  public ray center;
  public precision_type radius;
  public uint material_index;

  void get_sphere_uv(const point3 p, out precision_type u, out precision_type v) {
    float theta = acos(float(-p.y));
    float phi = atan2(float(-p.z), float(p.x)) + float(PI);

    u = precision_type(phi / (2.0f * float(PI)));
    v = precision_type(theta / float(PI));
  }

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

public struct Quad {
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

public struct HittableList {
  public uint first;
  public uint count;
}

internal struct Transform {
  internal matrix<precision_type, 3, 3> basis;
  internal vec3 offset;

  internal static Transform identity() {
    Transform transform;
    transform.basis = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
    transform.offset = vec3(0, 0, 0);
    return transform;
  }

  internal static void applyToHitRecord(const Transform local_to_world, inout hit_record rec, const ray world_ray) {
    vec3 outward_normal = rec.front_face ? rec.normal : -rec.normal;
    rec.p = local_to_world.offset + mul(local_to_world.basis,rec.p);
    outward_normal = normalize(mul(local_to_world.basis, outward_normal));
    rec.set_face_normal(world_ray, outward_normal);
  }
}

public struct Translate {
  public uint hittable_index;
  public vec3 offset;

  internal static Transform composeTranslate(const Transform parent, const vec3 offset) {
    Transform transform = parent;
    transform.offset = parent.offset + mul(parent.basis, offset);
    return transform;
  }


  internal static ray inverseTranslateRay(const ray r, const vec3 offset) {
    ray transformed;
    transformed.orig = r.orig - offset;
    transformed.dir = r.dir;
    transformed.tm = r.tm;
    return transformed;
  }
}

public struct RotateY {
  public uint hittable_index;
  public precision_type sin_theta;
  public precision_type cos_theta;

  internal static Transform composeRotateY(const Transform parent, precision_type sin_theta, precision_type cos_theta) {
    Transform transform;
    matrix<precision_type, 3, 3> rotate_y = {
      { cos_theta, 0, sin_theta },
      { 0, 1, 0 },
      { -sin_theta, 0, cos_theta },
    };
    transform.basis = mul(parent.basis, rotate_y);
    transform.offset = parent.offset;
    return transform;
  }

  internal static ray inverseRotateYRay(const ray r, precision_type sin_theta, precision_type cos_theta) {
    matrix<precision_type, 3, 3> inverse_rotate_y = {
      { cos_theta, 0, -sin_theta },
      { 0, 1, 0 },
      { sin_theta, 0, cos_theta },
    };

    ray transformed;
    transformed.orig = mul(inverse_rotate_y, r.orig);
    transformed.dir = mul(inverse_rotate_y, r.dir);
    transformed.tm = r.tm;
    return transformed;
  }
};

internal struct TraversalEntry {
  internal uint hittable_index;
  internal ray local_ray;
  internal Transform local_to_world;
};

public bool hit(
  StructuredBuffer<Hittable> world,
  uint hittable_count,
  ray r,
  interval ray_t,
  inout hit_record rec) {
  hit_record primitive_hit_rec;
  bool hit_anything = false;
  precision_type closest_so_far = ray_t.max;
  TraversalEntry stack[64];
  uint stack_size = 0;

  if (hittable_count == 0)
    return false;

  TraversalEntry root;
  root.hittable_index = hittable_count - 1;
  root.local_ray = r;
  root.local_to_world = Transform::identity();
  stack[stack_size++] = root;

  while (stack_size > 0) {
    TraversalEntry entry = stack[--stack_size];
    uint hittable_index = entry.hittable_index;
    bool hit_on_primitive = false;
    switch (world[hittable_index].type) {
    case HittableType::BvhNode:
      if (world[hittable_index].bbox.hit(entry.local_ray, interval(ray_t.min, closest_so_far))) {
        BvhNode node = reinterpret<BvhNode>(world[hittable_index].data);
        if (node.left == node.right) {
          if (stack_size < 64) {
            entry.hittable_index = node.left;
            stack[stack_size++] = entry;
          }
        } else {
          if (stack_size + 1 < 64) {
            TraversalEntry left_entry = entry;
            TraversalEntry right_entry = entry;
            left_entry.hittable_index = node.left;
            right_entry.hittable_index = node.right;
            stack[stack_size++] = left_entry;
            stack[stack_size++] = right_entry;
          }
        }
      }
      break;
    case HittableType::HittableList:
      if (world[hittable_index].bbox.hit(entry.local_ray, interval(ray_t.min, closest_so_far))) {
        HittableList list = reinterpret<HittableList>(world[hittable_index].data);
        const uint pushed_count = min(list.count, 64 - stack_size);
        for (uint child_offset = 0; child_offset < pushed_count; child_offset++) {
          TraversalEntry child_entry = entry;
          child_entry.hittable_index = list.first + child_offset;
          stack[stack_size++] = child_entry;
        }
      }
      break;
    // Transformations ---------------------------------------------------------------------------
    case HittableType::Translate:
      if (stack_size < 64) {
        Translate translate = reinterpret<Translate>(world[hittable_index].data);
        entry.hittable_index = translate.hittable_index;
        entry.local_ray = Translate::inverseTranslateRay(entry.local_ray, translate.offset);
        entry.local_to_world = Translate::composeTranslate(entry.local_to_world, translate.offset);
        stack[stack_size++] = entry;
      }
      break;
    case HittableType::RotateY:
      if (stack_size < 64) {
        RotateY rotate = reinterpret<RotateY>(world[hittable_index].data);
        entry.hittable_index = rotate.hittable_index;
        entry.local_ray = RotateY::inverseRotateYRay(entry.local_ray, rotate.sin_theta, rotate.cos_theta);
        entry.local_to_world = RotateY::composeRotateY(entry.local_to_world, rotate.sin_theta, rotate.cos_theta);
        stack[stack_size++] = entry;
      }
      break;
    // Primitives ---------------------------------------------------------------------------
    case HittableType::Sphere:
      {
        Sphere sphere = reinterpret<Sphere>(world[entry.hittable_index].data);
        hit_on_primitive = sphere.hit(entry.local_ray, interval(ray_t.min, closest_so_far), primitive_hit_rec);
      }
      break;
    case HittableType::Quad:
      {
        Quad quad = reinterpret<Quad>(world[entry.hittable_index].data);
        hit_on_primitive = quad.hit(entry.local_ray, interval(ray_t.min, closest_so_far), primitive_hit_rec);
      }
      break;
    default:
      break;
    }
    if (hit_on_primitive) {
      Transform::applyToHitRecord(entry.local_to_world, primitive_hit_rec, r);
      hit_anything = true;
      closest_so_far = primitive_hit_rec.t;
      rec = primitive_hit_rec;
    }
  }

  return hit_anything;
}
