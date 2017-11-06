/*
 * This file is part of ElasticFusion.
 *
 * Copyright (C) 2015 Imperial College London
 * 
 * The use of the code within this file and all code within files that 
 * make up the software that is ElasticFusion is permitted for 
 * non-commercial purposes only.  The full terms and conditions that 
 * apply to the code within this file are detailed within the LICENSE.txt 
 * file and at <http://www.imperial.ac.uk/dyson-robotics-lab/downloads/elastic-fusion/elastic-fusion-license/> 
 * unless explicitly stated.  By downloading this file you agree to 
 * comply with these terms.
 *
 * If you wish to use any of this code for commercial purposes then 
 * please email researchcontracts.engineering@imperial.ac.uk.
 *
 */

float max3(vec3 v)
{
  return max(v.x, max(v.y, v.z));
}

float min3(vec3 v)
{
  return min(v.x, min(v.y, v.z));
}

bool isHDRColorComplete(uvec2 hdr_packed)
{
  return bool(hdr_packed.y & 0xFFFF0000);
}

/* Packed byte layout of complete HDR color in uint2
 *
 * -----------   -----------
 * | GG | RR |   | WW | BB |
 * -----------   -----------
 *
 * Note: it is important to use simple cast from uint to float,
 * not uintBitsToFloat, for some reason this breaks when colors
 * are stored and then fetched from textures.
 *
 */

uvec2 packHDRColorComplete(vec4 hdr_unpacked_rgbw)
{
  uint rg = packUnorm2x16(hdr_unpacked_rgbw.xy);
  uint bw = packUnorm2x16(hdr_unpacked_rgbw.zw);
  return uvec2(rg, bw);
}

vec4 unpackHDRColorComplete(uvec2 hdr_packed)
{
  vec2 rg = unpackUnorm2x16(hdr_packed.x);
  vec2 bw = unpackUnorm2x16(hdr_packed.y);
  return vec4(rg, bw);
}

/* Packed byte layout of incomplete HDR color in uint2
 *
 * -----------   -----------
 * | BR | RG |   | WW | GB |
 * | lh | ll |   | 00 | hh |
 * -----------   -----------
 *
 */

uvec2 packHDRColorIncomplete(vec3 lo, vec3 hi)
{
  uint u1 = packUnorm4x8(vec4(lo, hi.x));
  uint u2 = packUnorm4x8(vec4(hi.yz, 0, 0));
  return uvec2(u1, u2);
}

/* Input:
 *   - exposure Et (3 channels, 0..1)
 *   - exposure time t
 *   - minimum irradiance at this exposure time (normalized)
 *   - maximum irradiance at this exposure time (normalized)
 *   - ln of black point
 *   - system dynamic range
 * Output:
 *   - packed HDR color/weight in 64-bit (as uvec2)
 */
uvec2 createHDRColor(vec3 Et, float t, vec3 e_min, vec3 e_max, vec3 bp, vec3 dr_sys)
{
  // Follows the convention of linearize.frag
  bvec3 OE = lessThan(Et, vec3(0.0));
  bvec3 UE = equal(Et, vec3(0.0));

  vec3 E = Et / t;

  // If any of the channels is saturated, then the color is invalid
  if (any(OE) || any(UE))
  {
    bvec3 PE = not(OE || UE);

    // Initialize boundaries
    //   * properly exposed -> [    E, E    ]
    //   * over exposed     -> [ Emax, 1    ]
    //   * under exposed    -> [    0, Emin ]

    // Logically, the bounds set by Emax and Emin should be exclusive.
    // But due to noise we assume inclusive. In other words, even

    vec3 lo = mix(vec3(0.0), e_max, OE);
    vec3 hi = mix(vec3(1.0), e_min, UE);

    vec3 e = (log(E) - bp) / dr_sys;
    lo = mix(lo, e, PE);
    hi = mix(hi, e, PE);

    return packHDRColorIncomplete(lo, hi);
  }

  float weight = t / 65535.0;

  return packHDRColorComplete(vec4(E, weight));
}

uvec2 fusePackedHDRColors(uvec2 hdr_packed1, uvec2 hdr_packed2)
{
  bool valid1 = isHDRColorComplete(hdr_packed1);
  bool valid2 = isHDRColorComplete(hdr_packed2);

  if (valid1 && valid2)
  {
    vec4 hdr1 = unpackHDRColorComplete(hdr_packed1);
    vec4 hdr2 = unpackHDRColorComplete(hdr_packed2);
    float w = hdr1.w + hdr2.w;
    // Proctect from weight overflow
    if (w < 1.0)
      return packHDRColorComplete(vec4(hdr1.xyz * hdr1.w / w + hdr2.xyz * hdr2.w / w, w));
    else
      return hdr_packed1;
  }
  else if (!valid1 && valid2)
  {
    return hdr_packed2;
  }
  else if (valid1 && !valid2)
  {
    return hdr_packed1;
  }
  else if (!valid1 && !valid2)
  {
    vec4 rlglblrh1 = unpackUnorm4x8(uint(hdr_packed1.x));
    vec4 ghbhwwww1 = unpackUnorm4x8(uint(hdr_packed1.y));
    vec4 rlglblrh2 = unpackUnorm4x8(uint(hdr_packed2.x));
    vec4 ghbhwwww2 = unpackUnorm4x8(uint(hdr_packed2.y));
    vec3 lo1 = rlglblrh1.xyz;
    vec3 lo2 = rlglblrh2.xyz;
    vec3 hi1 = vec3(rlglblrh1.w, ghbhwwww1.xy);
    vec3 hi2 = vec3(rlglblrh2.w, ghbhwwww2.xy);
    // TODO: be careful about mixing channels that have been properly exposed
    // TODO: is it possible that lo > hi
    vec3 lo = max(lo1, lo2);
    vec3 hi = min(hi1, hi2);
    return packHDRColorIncomplete(lo, hi);
  }
}

