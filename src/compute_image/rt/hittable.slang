implementing rt;

public struct hit_record {
  public point3 p;
  public vec3 normal;
  public Material mat;
  public precision_type t;
  public bool front_face;

  [mutating]
  public void set_face_normal(const ray r, const vec3 outward_normal) {
    // Sets the hit record normal vector.
    // NOTE: the parameter `outward_normal` is assumed to have unit length.

    front_face = dot(r.dir, outward_normal) < 0;
    normal = front_face ? outward_normal : -outward_normal;
  }
}

public interface hittable {
  bool hit(const ray r, interval ray_t, inout hit_record rec);
}