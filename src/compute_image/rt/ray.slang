implementing rt;

public struct ray {
  public point3 orig;
  public vec3 dir;
  public precision_type tm;

  public point3 at(precision_type t) { return orig + t * dir; }
};
