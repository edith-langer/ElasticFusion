#include "ExposureTimeController.h"
#include "ColorManager.h"
#include "Utils/Resolution.h"
#include "Utils/Stopwatch.h"

struct ExposureTimeController::ColorState
{
    union
    {
        uint16_t rgb[3];
        struct
        {
            uint16_t r;
            uint16_t g;
            uint16_t b;
        };
        uint8_t boundaries[6];
        struct
        {
            uint8_t rl;
            uint8_t gl;
            uint8_t bl;
            uint8_t rh;
            uint8_t gh;
            uint8_t bh;
        };
    };
    uint16_t w;
};

ExposureTimeController::ExposureTimeController(const ColorManager& colorMgr)
: colorMgr(colorMgr)
, activeTexture(0)
, currentExposureTime(0)
, awaitingChange(0)
, colorStateBuffer(nullptr)
{
    previousLuminanceTexture1.reset(new GPUTexture(Resolution::getInstance().width(), Resolution::getInstance().height(), GL_LUMINANCE32F_ARB, GL_LUMINANCE, GL_FLOAT));
    previousLuminanceTexture2.reset(new GPUTexture(Resolution::getInstance().width(), Resolution::getInstance().height(), GL_LUMINANCE32F_ARB, GL_LUMINANCE, GL_FLOAT));
    diffTexture.reset(new GPUTexture(Resolution::getInstance().width(), Resolution::getInstance().height(), GL_LUMINANCE32F_ARB, GL_LUMINANCE, GL_FLOAT));
    std::vector<pangolin::GlTexture*> targets1, targets2;
    targets1.push_back(diffTexture->texture); targets1.push_back(previousLuminanceTexture2->texture);
    targets2.push_back(diffTexture->texture); targets2.push_back(previousLuminanceTexture1->texture);
    computePack1.reset(new ComputePack(loadProgramFromFile("empty.vert", "exposure_diff.frag", "quad.geom"), targets1));
    computePack2.reset(new ComputePack(loadProgramFromFile("empty.vert", "exposure_diff.frag", "quad.geom"), targets2));
    colorStateBuffer = new ColorState[Resolution::getInstance().numPixels()];

    int min = colorMgr.minExposureTime();
    int max = colorMgr.maxExposureTime();
    MIN_IRRADIANCE.resize(3, max - min + 1);
    MAX_IRRADIANCE.resize(3, max - min + 1);
    for (int t = min; t <= max; ++t)
    {
        MIN_IRRADIANCE.col(t - min) = (colorMgr.normalizedMinIrradiance(t) * 255).cast<uint8_t>();
        MAX_IRRADIANCE.col(t - min) = (colorMgr.normalizedMaxIrradiance(t) * 255).cast<uint8_t>();
    }
}

ExposureTimeController::~ExposureTimeController()
{
    delete[] colorStateBuffer;
}

float ExposureTimeController::computeCurrentExposureTime(GPUTexture* luminanceInput, float nominalExposureTime)
{
    GPUTexture* previousLuminance;
    ComputePack* computePack;

    float scale = 0;
    if (currentExposureTime > 0)
        scale = std::log(nominalExposureTime / currentExposureTime) / 2;

    if (activeTexture == 1)
    {
        previousLuminance = previousLuminanceTexture2.get();
        computePack = computePack2.get();
    }
    else
    {
        previousLuminance = previousLuminanceTexture1.get();
        computePack = computePack1.get();
    }

    std::vector<pangolin::GlTexture*> inputs;
    inputs.push_back(luminanceInput->texture);
    inputs.push_back(previousLuminance->texture);
    std::vector<Uniform> uniforms;
    uniforms.push_back(Uniform("luminanceSampler", 0));
    uniforms.push_back(Uniform("previousLuminanceSampler", 1));
    uniforms.push_back(Uniform("exposureScale", scale));
    computePack->compute(inputs, &uniforms);

    activeTexture = (activeTexture + 1) % 2;

    // Nominal is the same as current, no exposure change is anticipated
    if (scale == 0)
    {
        awaitingChange = 0;
        currentExposureTime = nominalExposureTime;
        return currentExposureTime;
    }

    float avg = diffTexture->computeAverage();

    // Exposure change detected
    if (avg > 0.5 || awaitingChange++ > 2)
    {
        awaitingChange = 0;
        currentExposureTime = nominalExposureTime;
        return currentExposureTime;
    }

    // Exposure remains the same
    awaitingChange++;
    return currentExposureTime;
}

float ExposureTimeController::computeNextExposureTime(GPUTexture* colorState)
{
    if (awaitingChange)
        return -1;

    TICK("> Compute next exposure time");

    TICK(">> Download color state");

    auto num = Resolution::getInstance().numPixels();
    colorState->texture->Download(colorStateBuffer, GL_RG_INTEGER, GL_UNSIGNED_INT);

    TOCK(">> Download color state");

    TICK(">> Refinement");
    auto R = computeRefinementUtility(colorStateBuffer, num);
    TOCK(">> Refinement");

    TICK(">> Exploration");
    auto E = computeExplorationUtility(colorStateBuffer, num);
    TOCK(">> Exploration");

    Eigen::ArrayXd scores = E * 1 + R;

    Eigen::ArrayXd::Index max;
    scores.maxCoeff(&max);
    float nextExposureTime = max + colorMgr.minExposureTime();
    if (scores(max) == 0 || nextExposureTime == currentExposureTime)
        nextExposureTime = -1;

    TOCK("> Compute next exposure time");

    return nextExposureTime;
}

Eigen::ArrayXd ExposureTimeController::computeRefinementUtility(const ColorState* data, size_t num_pixels) const
{
    std::vector<double> H(colorMgr.maxExposureTime() - colorMgr.minExposureTime() + 2, 0);
    for (size_t i = 0; i < num_pixels; ++i)
    {
        const auto& color = data[i];

        if (color.w == 0)
            continue;

        double weight = 1.0 / colorStateBuffer[i].w;

        Eigen::Array3f rgb = Eigen::Array3f(color.r, color.g, color.b) / 65535;
        float min = colorMgr.minExposureTime(rgb);
        float max = colorMgr.maxExposureTime(rgb);
        H[min - colorMgr.minExposureTime()] -= weight;
        H[max - colorMgr.minExposureTime() + 1] += weight;
    }

    Eigen::ArrayXd utilities = Eigen::ArrayXd::Zero(colorMgr.maxExposureTime() - colorMgr.minExposureTime() + 1);
    for (int i = H.size() - 2; i >= 0; --i)
    {
      H[i] += H[i + 1];
      utilities [i] = H[i + 1] * (i + colorMgr.minExposureTime());
    }

    return utilities ;
}

Eigen::ArrayXd ExposureTimeController::computeExplorationUtility(const ColorState* data, size_t num_pixels) const
{
    Eigen::ArrayXd utilities = Eigen::ArrayXd::Zero(colorMgr.maxExposureTime() - colorMgr.minExposureTime() + 1);

    // Compute utility for every initialized incomplete color
    for (size_t i = 0; i < num_pixels; ++i)
    {
        const auto& color = data[i];

        if (color.w > 0 || color.b == 0)
            continue;

        Eigen::ArrayXf weights = Eigen::ArrayXf::Ones(colorMgr.maxExposureTime() - colorMgr.minExposureTime() + 1);

        // For each color channel
        for (size_t c = 0; c < 3; ++c)
        {
            uint8_t lo_bound = color.boundaries[c];
            uint8_t hi_bound = color.boundaries[c + 3];

            int width = hi_bound - lo_bound;
            if (width < 0)
                std::swap(lo_bound, hi_bound);

            if (width == 0)
            {
                // Exact value (lo and hi are the same)
                if (lo_bound == 0 || lo_bound  == 255)
                    // Black or white point
                    weights.fill(0);
                else
                    // Value should be within limits
                    weights *= MAX_IRRADIANCE.row(c).min(hi_bound).cwiseEqual(MIN_IRRADIANCE.row(c).max(lo_bound)).cast<float>();
            }
            else
            {
                // TODO change this in the end when refactoring is completely done
                float inverse = 1.f / std::abs(width);
                weights *= (MAX_IRRADIANCE.row(c).min(hi_bound).cast<float>() - MIN_IRRADIANCE.row(c).max(lo_bound).cast<float>()).max(0.0) * inverse;
            }
        }

        utilities += weights.cast<double>();
    }

    return utilities;
}

