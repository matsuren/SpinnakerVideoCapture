
#include "spincamera.hpp"

#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <sstream>
#include <string>

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
SpinCam::SpinCam(CameraPtr pCam_) {
  try {
    pCam = pCam_;
    pCam->Init();
    pCam->PixelFormat.SetValue(PixelFormat_BayerGB8);
    // Image acquisition
    pCam->AcquisitionMode.SetValue(AcquisitionMode_Continuous);
    // Newest frame only to reduce latency
    pCam->TLStream.StreamBufferHandlingMode.SetValue(
        StreamBufferHandlingMode_NewestOnly);

    std::cout << "Pixel format:" << pCam->PixelFormat.ToString() << std::endl;
    std::cout << "Device model:" << pCam->DeviceModelName.ToString()
              << std::endl;
    std::cout << "Serial number:" << pCam->DeviceSerialNumber.ToString()
              << std::endl;

    // White balance
    setWhiteBalanceRatio(1.06);
  } catch (Spinnaker::Exception& e) {
    std::cout << "Error: " << e.what() << std::endl;
  }
}

SpinCam::~SpinCam() {
  if (pCam) release();
}

bool SpinCam::read(cv::Mat& img_) {
  grab();
  bool ret = retrieve(img_);
  return ret;
}

void SpinCam::grab() {
  if (!isCapturing) {
    // Start capture
    displaySetting();
    pCam->BeginAcquisition();
    isCapturing = true;
  }

  if (isSoftwareTrigger) pCam->TriggerSoftware();
  isGrab = true;
}

bool SpinCam::retrieve(cv::Mat& img_) {
  if (!isGrab & isSoftwareTrigger) {
    std::cout << "Software trigger is set. Please grab() first." << std::endl;
    return false;
  }

  ImagePtr pResultImage = pCam->GetNextImage();
  if (pResultImage->IsIncomplete()) {
    return false;
  }

  if (true) {
    int width = pResultImage->GetWidth();
    int height = pResultImage->GetHeight();
    cv::Mat img(height, width, CV_8UC1, pResultImage->GetData());
    cv::cvtColor(img, img_, cv::COLOR_BayerGR2BGR);
  } else {
    pConverted = pResultImage->Convert(PixelFormat_BGR8, HQ_LINEAR);
    int width = pConverted->GetWidth();
    int height = pConverted->GetHeight();
    img_ = cv::Mat(height, width, CV_8UC3, pConverted->GetData());
  }

  pResultImage->Release();
  return true;
}

void SpinCam::setFrameRate(double fps) {
  pCam->TriggerMode.SetValue(TriggerMode_Off);
  pCam->TriggerSource.SetValue(TriggerSource_Line0);
  setFrameRateAuto(false);
  std::cout << "Set framerate:" << fps << std::endl;
  pCam->AcquisitionFrameRate.SetValue(fps);
  isSoftwareTrigger = false;
}

void SpinCam::setROI(int offset_x, int offset_y, int width, int height) {
  if (pCam->OffsetX.GetValue() < offset_x) {
    int _width = width / 32;
    pCam->Width.SetValue(_width * 32);
    int _height = height / 2;
    pCam->Height.SetValue(_height * 2);
    pCam->OffsetX.SetValue(offset_x);
    pCam->OffsetY.SetValue(offset_y);
  } else {
    pCam->OffsetX.SetValue(offset_x);
    pCam->OffsetY.SetValue(offset_y);
    int _width = width / 32;
    pCam->Width.SetValue(_width * 32);
    int _height = height / 2;
    pCam->Height.SetValue(_height * 2);
  }
}

void SpinCam::release() {
  if (isCapturing) pCam->EndAcquisition();
  pCam->DeInit();
  pCam = nullptr;
}
void SpinCam::setSoftwareTrigger() {
  std::cout << "Set softwareTrigger" << std::endl;
  pCam->TriggerMode.SetValue(TriggerMode_Off);
  pCam->TriggerSource.SetValue(TriggerSource_Software);
  pCam->TriggerMode.SetValue(TriggerMode_On);
  isSoftwareTrigger = true;
}

void SpinCam::displaySetting() {
  std::cout << "#### Current setting ####" << std::endl;
  std::cout << "Width x Height, (offsetX x offsetY):\n"
            << pCam->Width.ToString() << " x " << pCam->Height.ToString()
            << ", (" << pCam->OffsetX.ToString() << " x "
            << pCam->OffsetY.ToString() << ")" << std::endl;

  if (isSoftwareTrigger) {
    std::cout << "Software trigger" << std::endl;
  } else {
    std::cout << "FPS:" << pCam->AcquisitionFrameRate.ToString() << std::endl;
  }
}

void SpinCam::setFrameRateAuto(bool flag) {
  int64_t value;

  // Turning AcquisitionFrameRateAuto on or off
  CEnumerationPtr ptrFrameRateAuto =
      pCam->GetNodeMap().GetNode("AcquisitionFrameRateAuto");
  if (!IsAvailable(ptrFrameRateAuto) || !IsWritable(ptrFrameRateAuto)) {
    std::cout << "Unable to set AcquisitionFrameRateAuto..." << std::endl
              << std::endl;
    return;
  }
  if (flag) {
    CEnumEntryPtr ptrFrameRateAutoMode =
        ptrFrameRateAuto->GetEntryByName("Continuous");
    if (!IsAvailable(ptrFrameRateAutoMode) ||
        !IsReadable(ptrFrameRateAutoMode)) {
      std::cout << "Unable to set AcquisitionFrameRateAuto to Continuous. "
                   "Aborting..."
                << std::endl
                << std::endl;
      return;
    }
    value = ptrFrameRateAutoMode->GetValue();
  } else {
    CEnumEntryPtr ptrFrameRateAutoMode =
        ptrFrameRateAuto->GetEntryByName("Off");
    if (!IsAvailable(ptrFrameRateAutoMode) ||
        !IsReadable(ptrFrameRateAutoMode)) {
      std::cout << "Unable to set AcquisitionFrameRateAuto to OFF. Aborting..."
                << std::endl
                << std::endl;
      return;
    }
    // Set value
    value = ptrFrameRateAutoMode->GetValue();
  }
  ptrFrameRateAuto->SetIntValue(value);
}

void SpinCam::setWhiteBalanceRatio(double val) {
  pCam->BalanceRatioSelector.SetValue(BalanceRatioSelector_Red);
  pCam->BalanceRatio.SetValue(val);
}

///////////////////////////////////////////////////////
// SpinMultiCam
///////////////////////////////////////////////////////
void SpinMultiCam::addCamera(CameraPtr& cam) {
  // Only software trigger is possible
  SpinCamPtr cam_ptr = std::make_shared<SpinCam>(cam);
  cam_ptr->setSoftwareTrigger();
  spincams.push_back(cam_ptr);
}

bool SpinMultiCam::read(std::vector<cv::Mat>& imgs) {
  grab();
  bool ret = retrieve(imgs);
  return ret;
}

void SpinMultiCam::grab() {
  for (auto& it : spincams) {
    it->grab();
  }
}

bool SpinMultiCam::retrieve(std::vector<cv::Mat>& imgs) {
  imgs.clear();
  bool ret = true;
  for (auto& it : spincams) {
    cv::Mat tmpimg;
    ret &= it->retrieve(tmpimg);
    imgs.push_back(tmpimg);
  }
  return ret;
}

void SpinMultiCam::release() {
  for (auto& it : spincams) {
    it->release();
  }
}
