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

layout (location = 0) in vec4 position;
layout (location = 1) in uvec4 color;
layout (location = 2) in vec4 normal;

uniform mat4 MVP;
uniform mat4 pose;
uniform float threshold;
uniform int colorType;

out vec4 vColor;
flat out int tonemap;

#include "color.glsl"

void main()
{
    if(position.w > threshold)
    {
        tonemap = 0;

        if(colorType == 1)
        {
            vColor = vec4(normal.xyz, 1.0);
        }
        else if(colorType == 2)
        {
            // Assume color is complete. If it is fact incomplete, we get garbage in RGB, but
            // that does not matter. Weight will be valid either way.
            vec4 hdr = unpackHDRColorComplete(color.xy);
            if (hdr.w == 0)
            {
              vColor = vec4(1.0, 0.0, 0.0, 1.0);
              tonemap = 0;
            }
            else
            {
              vColor = vec4(hdr.xyz, 1.0);
              tonemap = 1;
            }
        }
        else
        {
            vColor = vec4((vec3(.5f, .5f, .5f) * abs(dot(normal.xyz, vec3(1.0, 1.0, 1.0)))) + vec3(0.1f, 0.1f, 0.1f), 1.0f);
        }
	    gl_Position = MVP * pose * vec4(position.xyz, 1.0);
    }
    else
    {
        gl_Position = vec4(-10, -10, 0, 1);
    }
}
