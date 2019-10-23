#pragma once
#include <opencv2/core.hpp>

#include "SpinGenApi/SpinnakerGenApi.h"
#include "Spinnaker.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

class SpinCam;
using SpinCamPtr = std::shared_ptr<SpinCam>;

///////////////////////////////////////////////////////
// SpinCam
///////////////////////////////////////////////////////
class SpinCam {
public:
  SpinCam(CameraPtr pCam_);

  ~SpinCam();

  bool read(cv::Mat& img_);
  void grab();
  bool retrieve(cv::Mat& img_);

  void setFrameRate(double fps);
  void setROI(int offset_x, int offset_y, int width, int height);
  void release();
  void setSoftwareTrigger();
  void displaySetting();
  void setFrameRateAuto(bool flag);

private:
  void setWhiteBalanceRatio(double val);

  CameraPtr pCam;
  bool isGrab = false;
  bool isSoftwareTrigger = false;
  bool isCapturing = false;
  ImagePtr pConverted;  // save in memory
};

///////////////////////////////////////////////////////
// SpinMultiCam
///////////////////////////////////////////////////////
class SpinMultiCam {
public:
  void addCamera(CameraPtr& cam);

  bool read(std::vector<cv::Mat>& imgs);

  void grab();
  bool retrieve(std::vector<cv::Mat>& imgs);

  void release();

private:
  std::vector<SpinCamPtr> spincams;
};
