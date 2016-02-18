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

uvec2 encodeColor(vec3 c)
{
  uvec2 encoded;
  encoded.x = packUnorm2x16(c.xy);
  encoded.y = packUnorm2x16(vec2(c.z, c.z));
  return encoded;
}

vec3 decodeColor(uvec2 c)
{
  vec3 decoded;
  decoded.xy = unpackUnorm2x16(c.x);
  decoded.z = unpackUnorm2x16(c.y).x;
  return decoded;
}
