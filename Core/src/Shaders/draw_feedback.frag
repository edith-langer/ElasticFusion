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

#version 430 core

in vec4 vColor;
flat in int tonemap;

uniform float exposureTime;

out vec4 FragColor;

#include "tonemap.glsl"

void main()
{
    if(tonemap == 1)
      FragColor = vec4(gamma(tonemapping(vColor.xyz * exposureTime)), 1.0f);
    else
      FragColor = vColor;
}
