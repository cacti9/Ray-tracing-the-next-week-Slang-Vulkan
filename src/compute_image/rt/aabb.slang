implementing rt;

public struct aabb {
  public interval x, y, z;

  public interval axis_interval(int n) {
    if (n == 1)
      return y;
    if (n == 2)
      return z;
    return x;
  }

  public bool hit(const ray r, interval ray_t) {
    const point3 ray_orig = r.orig;
    const vec3 ray_dir = r.dir;

    for (int axis = 0; axis < 3; axis++) {
      interval ax = axis_interval(axis);
      precision_type adinv = 1.0 / ray_dir[axis];

      precision_type t0 = (ax.min - ray_orig[axis]) * adinv;
      precision_type t1 = (ax.max - ray_orig[axis]) * adinv;

      if (t0 < t1) {
        if (t0 > ray_t.min)
          ray_t.min = t0;
        if (t1 < ray_t.max)
          ray_t.max = t1;
      } else {
        if (t1 > ray_t.min)
          ray_t.min = t1;
        if (t0 < ray_t.max)
          ray_t.max = t0;
      }

      if (ray_t.max <= ray_t.min)
        return false;
    }
    return true;
  }
}
