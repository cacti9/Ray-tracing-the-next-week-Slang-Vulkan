implementing rt;

public struct ray {
  public point3 orig;
  public vec3 dir;

  point3 at(float t) { return orig + t * dir; }
};
