implementing rt;

public enum TextureType {
  SolidColor,
  Checker,
};

public static const uint TEXTURE_DATA_SIZE = 4;
public static const uint TEXTURE_EVAL_MAX_DEPTH = 16;

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

public color texture_value(StructuredBuffer<Texture> textures, uint texture_index, precision_type u, precision_type v, point3 p) {
  uint current_index = texture_index;

  for (uint depth = 0; depth < TEXTURE_EVAL_MAX_DEPTH; depth++) {
    Texture texture = textures[current_index];
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
    default:
      return color(1.0, 0.0, 1.0);
    }
  }

  return color(1.0, 0.0, 1.0);
}
