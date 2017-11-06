#version 430 core

in vec2 texcoord;

layout (location = 0) out uvec2 outHDR;
layout (location = 1) out vec4 outRadiance;

uniform sampler2D exposureSampler;
uniform float exposureTime;
uniform vec3 minIrradiance;
uniform vec3 maxIrradiance;
uniform vec3 blackPoint;
uniform vec3 drSys;

#include "color.glsl"

void main()
{
    vec3 X = texture(exposureSampler, texcoord.xy).rgb;
    outHDR.xy = createHDRColor(X, exposureTime, minIrradiance, maxIrradiance, blackPoint, drSys);

    if (all(greaterThan(X, vec3(0))))
    {
        // 7 is because ln(bp) = ln(0.000968231) ~= -7
        outRadiance = vec4(vec3(1.0) + log(X / exposureTime) / 7, 1);
    }
    else
    {
        outRadiance = vec4(0, 0, 0, 1);
    }
}
