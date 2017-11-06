#ifndef COLOR_MANAGER_H_
#define COLOR_MANAGER_H_

#include <memory>

#include "GPUTexture.h"
#include "Shaders/ComputePack.h"

class ColorManager
{
    public:
        ColorManager(const std::string& crfFile, const std::string& vgnFile,
                     float minExposureTime, float maxExposureTime,
                     uint8_t minValidIntensity, uint8_t maxValidIntensity);

        ~ColorManager();

        void linearize(GPUTexture* rgbInput);

        void computeRadianceAndHDR(float exposureTime);

        GPUTexture* exposureTex() { return exposureTexture.get(); }

        GPUTexture* luminanceTex() { return luminanceTexture.get(); }

        GPUTexture* hdrTex() { return hdrTexture.get(); }

        GPUTexture* radianceTex() { return radianceTexture.get(); }

        /** Get black point irradiance */
        const Eigen::Array3f& black() const
        {
            return blackPoint;
        }

        /** Get log of black point irradiance */
        const Eigen::Array3f& logBlack() const
        {
            return logBlackPoint;
        }

        /** Get white point irradiance */
        const Eigen::Array3f& white() const
        {
            return whitePoint;
        }

        /** Get log of white point irradiance */
        const Eigen::Array3f& logWhite() const
        {
            return logWhitePoint;
        }

        /** Get minimum irradiance at given exposure time */
        Eigen::Array3f minIrradiance(float t) const
        {
            return minExposure / t;
        }

        /** Get normalized minimum irradiance at given exposure time */
        Eigen::Array3f normalizedMinIrradiance(float t) const
        {
            return drSysInverse * (std::log(maxT) - std::log(t));
        }

        /** Get maximum irradiance at given exposure time */
        Eigen::Array3f maxIrradiance(float t) const
        {
            return maxExposure / t;
        }

        /** Get normalized maximum irradiance at given exposure time.
          * White point maps to 1, i.e. for the minimum possible exposure time this function returs ones. */
        Eigen::Array3f normalizedMaxIrradiance(float t) const
        {
            return drSysInverse * (dr + Eigen::Array3f(std::log(maxT) - std::log(t)));
        }

        /** Get the dynamic range of the camera. */
        const Eigen::Array3f& dynamicRange() const
        {
            return dr;
        }

        /** Get the system (effective) dynamic range of the camera. */
        const Eigen::Array3f& effectiveDynamicRange() const
        {
            return drSys;
        }

        /** Get minimum exposure time supported by the system. */
        float minExposureTime() const
        {
            return minT;
        }

        /** Get minimum exposure time at which given irradiance can be distinguished from noise. */
        float minExposureTime(const Eigen::Array3f& irradiance) const
        {
            return std::max(minT, std::ceil((minExposure / irradiance).maxCoeff()));
        }

        /** Get maximum exposure time supported by the system. */
        float maxExposureTime() const
        {
            return maxT;
        }

        /** Get maximum exposure time at which given irradiance can be observed without saturation. */
        float maxExposureTime(const Eigen::Array3f& irradiance) const
        {
            return std::min(maxT, std::floor((maxExposure / irradiance).minCoeff()));
        }

    private:

        std::unique_ptr<GPUTexture> exposureTexture;
        std::unique_ptr<GPUTexture> luminanceTexture;

        std::unique_ptr<GPUTexture> crfLUT;
        std::unique_ptr<GPUTexture> vignetteLUT;
  
        std::unique_ptr<ComputePack> linearizeComputePack;

        std::unique_ptr<GPUTexture> hdrTexture;
        std::unique_ptr<GPUTexture> radianceTexture;

        std::unique_ptr<ComputePack> hdrComputePack;

        const float minT;                  ///< Minimum exposure time
        const float maxT;                  ///< Maximum exposure time
        const uint8_t minValidIntensity;
        const uint8_t maxValidIntensity;
        Eigen::Array3f minExposure;        ///< Minimum valid exposure (inclusive), everything below is noise
        Eigen::Array3f maxExposure;        ///< Maximum valid exposure (inclusive), everything above is saturated
        Eigen::Array3f blackPoint;
        Eigen::Array3f logBlackPoint;
        Eigen::Array3f whitePoint;
        Eigen::Array3f logWhitePoint;
        Eigen::Array3f dr;                 ///< Dynamic range of a single frame
        Eigen::Array3f drSys;              ///< Effective dynamic range of the camera
        Eigen::Array3f drSysInverse;       ///< Inverse of the effective dynamic range of the camera

};

#endif /* COLOR_MANAGER_H_ */

