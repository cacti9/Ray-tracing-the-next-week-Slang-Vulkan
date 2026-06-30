implementing rt;

public typealias vec3 = float3;
public typealias point3 = float3;
public typealias color = float3;
public uint float3_to_packed_uint(float3 x) {
  x = clamp(x, 0.0, 1.0);
  return uint(255.999*x.b) | (uint(255.999*x.g) << 8) | (uint(255.999*x.r) << 16) | (255 << 24);
}
