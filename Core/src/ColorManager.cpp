#include "ColorManager.h"

#include <radical/radiometric_response.h>
#include <radical/vignetting_response.h>

ColorManager::ColorManager(const std::string& crfFile, const std::string& vgnFile,
                           float minExposureTime, float maxExposureTime,
                           uint8_t minValidIntensity, uint8_t maxValidIntensity)
: minT(minExposureTime)
, maxT(maxExposureTime)
, minValidIntensity(minValidIntensity)
, maxValidIntensity(maxValidIntensity)
{
    radical::RadiometricResponse radiometricResponse(crfFile);
    radical::VignettingResponse vignettingResponse(vgnFile);

    auto minBGR = radiometricResponse.inverseMap(cv::Vec3b(minValidIntensity, minValidIntensity, minValidIntensity));
    auto maxBGR = radiometricResponse.inverseMap(cv::Vec3b(maxValidIntensity, maxValidIntensity, maxValidIntensity));

    minExposure << minBGR[2], minBGR[1], minBGR[0];
    maxExposure << maxBGR[2], maxBGR[1], maxBGR[0];

    dr = maxExposure.log() - minExposure.log();
    drSys = dr + std::log(maxExposureTime / minExposureTime);
    drSysInverse = 1.0 / drSys;

    blackPoint = minExposure / maxExposureTime;
    logBlackPoint = blackPoint.log();
    whitePoint = maxExposure / minExposureTime;
    logWhitePoint = whitePoint.log();

    exposureTexture.reset(new GPUTexture(Resolution::getInstance().width(), Resolution::getInstance().height(), GL_RGBA32F, GL_RGBA, GL_FLOAT));
    luminanceTexture.reset(new GPUTexture(Resolution::getInstance().width(), Resolution::getInstance().height(), GL_LUMINANCE32F_ARB, GL_LUMINANCE, GL_FLOAT));
    // Inverse CRF and vignetting look-up tables. Note that radical gives us BGR, so inside shader we need to take care of this.
    crfLUT.reset(new GPUTexture(256, 1, GL_RGB32F, GL_RGB, GL_FLOAT)); crfLUT->texture->SetLinear();
    crfLUT->texture->Upload(radiometricResponse.getInverseResponse().data, GL_RGB, GL_FLOAT);
    vignetteLUT.reset(new GPUTexture(Resolution::getInstance().width(), Resolution::getInstance().height(), GL_RGB32F, GL_RGB, GL_FLOAT));
    vignetteLUT->texture->Upload(vignettingResponse.getResponse().data, GL_RGB, GL_FLOAT);
    std::vector<pangolin::GlTexture*> targets1;
    targets1.push_back(exposureTexture->texture); targets1.push_back(luminanceTexture->texture);
    linearizeComputePack.reset(new ComputePack(loadProgramFromFile("empty.vert", "linearize.frag", "quad.geom"), targets1));
    // Create HDR colors
    radianceTexture.reset(new GPUTexture(Resolution::getInstance().width(), Resolution::getInstance().height(), GL_RGBA32F, GL_RGB, GL_FLOAT, false, true));
    hdrTexture.reset(new GPUTexture(Resolution::getInstance().width(), Resolution::getInstance().height(), GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT));
    std::vector<pangolin::GlTexture*> targets2;
    targets2.push_back(hdrTexture->texture); targets2.push_back(radianceTexture->texture);
    hdrComputePack.reset(new ComputePack(loadProgramFromFile("empty.vert", "create_hdr.frag", "quad.geom"), targets2));
}

ColorManager::~ColorManager()
{
}

void ColorManager::linearize(GPUTexture* rgbInput)
{
    std::vector<pangolin::GlTexture*> inputs;
    inputs.push_back(rgbInput->texture);
    inputs.push_back(crfLUT->texture);
    inputs.push_back(vignetteLUT->texture);
    std::vector<Uniform> uniforms;
    uniforms.push_back(Uniform("rgbSampler", 0));
    uniforms.push_back(Uniform("crfSampler", 1));
    uniforms.push_back(Uniform("vignetteSampler", 2));
    uniforms.push_back(Uniform("minExposure", minExposure));
    uniforms.push_back(Uniform("maxExposure", maxExposure));
    linearizeComputePack->compute(inputs, &uniforms);
}

void ColorManager::computeRadianceAndHDR(float exposureTime)
{
    std::vector<pangolin::GlTexture*> inputs;
    inputs.push_back(exposureTexture->texture);
    std::vector<Uniform> uniforms;
    uniforms.push_back(Uniform("exposureSampler", 0));
    uniforms.push_back(Uniform("exposureTime", exposureTime));
    uniforms.push_back(Uniform("minIrradiance", normalizedMinIrradiance(exposureTime)));
    uniforms.push_back(Uniform("maxIrradiance", normalizedMaxIrradiance(exposureTime)));
    uniforms.push_back(Uniform("blackPoint", logBlack()));
    uniforms.push_back(Uniform("drSys", effectiveDynamicRange()));
    hdrComputePack->compute(inputs, &uniforms);
}

