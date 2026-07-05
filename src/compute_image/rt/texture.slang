implementing rt;

public enum TextureType {
  SolidColor,
  Checker,
  Image,
  Noise,
};

public static const uint TEXTURE_DATA_SIZE = RT_PRECISION_IS_DOUBLE ? 8 : 4;
public static const uint TEXTURE_EVAL_MAX_DEPTH = 16;
public static const uint MAX_IMAGE_TEXTURES = 16;
public static const uint PERLIN_POINT_COUNT = 256;

public struct Texture {
  public TextureType type;
  public uint[TEXTURE_DATA_SIZE] data;
};

public struct SolidColor {
  public color albedo;
}

public struct CheckerTexture {
  public precision_type inv_scale;
  public uint even_texture_index;
  public uint odd_texture_index;
}

public struct ImageTexture {
  public uint image_index;
}

public struct Perlin {
  public precision_type[PERLIN_POINT_COUNT*3] randvec_unpacked;
  public uint[PERLIN_POINT_COUNT] perm_x;
  public uint[PERLIN_POINT_COUNT] perm_y;
  public uint[PERLIN_POINT_COUNT] perm_z;

  public precision_type noise(const point3 p) {
    precision_type u = p.x - floor(p.x);
    precision_type v = p.y - floor(p.y);
    precision_type w = p.z - floor(p.z);

    int i = int(floor(p.x));
    int j = int(floor(p.y));
    int k = int(floor(p.z));
    vec3 c[2][2][2];

    for (int di = 0; di < 2; di++) {
      for (int dj = 0; dj < 2; dj++) {
        for (int dk = 0; dk < 2; dk++) {
          uint perm_index = 
            perm_x[(i + di) & 255] ^
            perm_y[(j + dj) & 255] ^
            perm_z[(k + dk) & 255];
          c[di][dj][dk].x = randvec_unpacked[3*perm_index];
          c[di][dj][dk].y = randvec_unpacked[3*perm_index+1];
          c[di][dj][dk].z = randvec_unpacked[3*perm_index+2];
        }
      }
    }

    return perlin_interp(c, u, v, w);
  }
  precision_type turb(const point3 p, int depth) {
    precision_type accum = 0.0;
    point3 temp_p = p;
    precision_type weight = 1.0;

    for (int i = 0; i < depth; i++) {
      accum += weight * noise(temp_p);
      weight *= 0.5;
      temp_p *= 2;
    }

    return abs(accum);
  }

  static precision_type perlin_interp(vec3 c[2][2][2], precision_type u, precision_type v, precision_type w) {
    precision_type uu = u*u*(3-2*u);
    precision_type vv = v*v*(3-2*v);
    precision_type ww = w*w*(3-2*w);
    precision_type accum = 0.0;

    for (int i=0; i < 2; i++)
      for (int j=0; j < 2; j++)
        for (int k = 0; k < 2; k++) {
          vec3 weight_v = { u - i, v - j, w - k };
          accum += (i*uu + (1-i)*(1-uu))
                 * (j*vv + (1-j)*(1-vv))
                 * (k*ww + (1-k)*(1-ww))
                 * dot(c[i][j][k], weight_v);
        }

    return accum;
  }
}

public struct NoiseTexture {
  public precision_type scale;
}

public struct TextureContext {
  public StructuredBuffer<Texture> textures;
  public Texture2D<float4> image_textures[MAX_IMAGE_TEXTURES];
  public SamplerState image_sampler;
  public StructuredBuffer<Perlin> perlins;
}

public color texture_value(
  TextureContext texture_context,
  uint texture_index,
  precision_type u,
  precision_type v,
  point3 p
) {
  uint current_index = texture_index;

  for (uint depth = 0; depth < TEXTURE_EVAL_MAX_DEPTH; depth++) {
    Texture texture = texture_context.textures[current_index];
    switch (texture.type) {
    case TextureType::SolidColor:
      SolidColor solid = reinterpret<SolidColor>(texture.data);
      return solid.albedo;
    case TextureType::Checker:
      CheckerTexture checker = reinterpret<CheckerTexture>(texture.data);
      int xInteger = int(floor(checker.inv_scale * p.x));
      int yInteger = int(floor(checker.inv_scale * p.y));
      int zInteger = int(floor(checker.inv_scale * p.z));
      bool isEven = ((xInteger + yInteger + zInteger) % 2) == 0;
      current_index = isEven ? checker.even_texture_index : checker.odd_texture_index;
      break;
    case TextureType::Image:
      ImageTexture image = reinterpret<ImageTexture>(texture.data);
      uint image_index = min(image.image_index, MAX_IMAGE_TEXTURES - 1);
      return texture_context.image_textures[NonUniformResourceIndex(image_index)].SampleLevel(
        texture_context.image_sampler,
        float2(float(u), float(1.0 - v)),
        0.0
      ).rgb;
    case TextureType::Noise:
      NoiseTexture noise = reinterpret<NoiseTexture>(texture.data);
      precision_type phase = noise.scale * p.z + 10 * texture_context.perlins[0].turb(p, 7);
      return color(.5, .5, .5) * (1 + precision_type(sin(float(phase))));
    default:
      return color(1.0, 0.0, 1.0);
    }
  }

  return color(1.0, 0.0, 1.0);
}
