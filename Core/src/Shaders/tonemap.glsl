float luminance(vec3 rgb)
{
  return 0.2126 * rgb.r + 0.7152 * rgb.g + 0.0722 * rgb.b;
}

vec3 tonemapping(vec3 hdr)
{
  float L = luminance(hdr);
  float whitePoint2 = 9;
  float scale = float(1.0 * (1.0 + L / whitePoint2) / (L + 1.0));
  return hdr * scale;
}

vec3 gamma(vec3 color)
{
  return pow(color, vec3(1.0 / 1.8));
}
