#version 430 core

in vec2 texcoord;

layout (location = 0) out vec3 outX;
layout (location = 1) out float outLuminance;

uniform sampler2D rgbSampler;
uniform sampler2D crfSampler;
uniform sampler2D vignetteSampler;
uniform vec3 minExposure;
uniform vec3 maxExposure;

void main()
{
    vec3 rgb = texture(rgbSampler, texcoord.xy).rgb;
    float red = texture(crfSampler, vec2(rgb.r, 0)).b;
    float green = texture(crfSampler, vec2(rgb.g, 0)).g;
    float blue = texture(crfSampler, vec2(rgb.b, 0)).r;
    vec3 X = vec3(red, green, blue);

    bvec3 UE = lessThan(X, minExposure);
    bvec3 OE = greaterThan(X, maxExposure);

    vec3 vignette = texture(vignetteSampler, texcoord.xy).bgr;
    outX = vec3(red, green, blue) / vignette;

    if (any(UE) || any(OE))
    {
        // Set underexposed channels to 0 and overexposed channels to -1
        outX = mix(outX, vec3( 0.0), UE);
        outX = mix(outX, vec3(-1.0), OE);
        outLuminance = 0;
    }
    else
    {
        outLuminance = 0.2126 * outX.r + 0.7152 * outX.g + 0.0722 * outX.b;
    }
}

