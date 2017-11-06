#version 430 core

in vec2 texcoord;

layout (location = 0) out float outDiff;
layout (location = 1) out float outNewPreviousLuminance;

uniform sampler2D luminanceSampler;
uniform sampler2D previousLuminanceSampler;

uniform float exposureScale;

void main()
{
    float currentLuminance = texture(luminanceSampler, texcoord.xy).r;
    float previousLuminance = texture(previousLuminanceSampler, texcoord.xy).r;

    if (currentLuminance > 0.1 && previousLuminance > 0.1)
    {
        float scale = log(currentLuminance / previousLuminance);
        outDiff = (int(exposureScale > 0) ^ int(scale > exposureScale)) > 0 ? 0 : 1;
    }
    else
    {
        outDiff = 0.5;
    }

    outNewPreviousLuminance = currentLuminance;
}
