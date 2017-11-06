#ifndef EXPOSURE_TIME_CONTROLLER_H_
#define EXPOSURE_TIME_CONTROLLER_H_

#include <memory>

#include "GPUTexture.h"
#include "Shaders/ComputePack.h"

class ColorManager;

/** Keeps track of the actual exposure time of the camera and makes decisions
  * which exposure time to use in the next frame based on the current state of
  * the map. */
class ExposureTimeController
{
    public:

        ExposureTimeController(const ColorManager& colorMgr);

        ~ExposureTimeController();

        /** Compute current actual exposure time.
          * Due to the control lag of the camera, the exposure time commanded to the camera (nominal) does
          * not necessarily match the actual exposure time. Given the current luminance and nominal exposure
          * time (and cached luminance and exposure time of the previous frame) this function decides what
          * is more likely, that the actual exposure time is still unchanged, or has changed as commanded
          * to the camera.
          * \param[in] luminanceINput texture with current luminance map
          * \param[in] nominalExposureTime exposure time that was commanded to the camera
          * \return actual exposure time of the current frame */
        float computeCurrentExposureTime(GPUTexture* luminanceInput, float nominalExposureTime);

        /** Decide which exposure time will bring most information gain in the next frame.
          * \param[in] colorState texture with current rendered color state of the map
          * \return best next exposure time or -1 if no change is needed */
        float computeNextExposureTime(GPUTexture* colorState);

    private:

        struct ColorState;

        Eigen::ArrayXd computeRefinementUtility(const ColorState* data, size_t num_pixels) const;

        Eigen::ArrayXd computeExplorationUtility(const ColorState* data, size_t num_pixels) const;

        const ColorManager& colorMgr;

        std::unique_ptr<GPUTexture>  previousLuminanceTexture1;
        std::unique_ptr<GPUTexture>  previousLuminanceTexture2;
        std::unique_ptr<GPUTexture>  diffTexture;
        std::unique_ptr<ComputePack> computePack1;
        std::unique_ptr<ComputePack> computePack2;

        // (Normalized) detectable range for every supported exposure time, lower and upper bounds
        Eigen::Array<uint8_t, 3, Eigen::Dynamic> MIN_IRRADIANCE;
        Eigen::Array<uint8_t, 3, Eigen::Dynamic> MAX_IRRADIANCE;

        int activeTexture;
        float currentExposureTime;
        int awaitingChange;

        ColorState* colorStateBuffer;

};

#endif /* EXPOSURE_TIME_CONTROLLER_H_ */

