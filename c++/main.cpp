#include <chrono>
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
using micro = std::chrono::microseconds;

class SpinManager {
public:
  SpinManager() {
    system = System::GetInstance();
    const LibraryVersion spinnakerLibraryVersion = system->GetLibraryVersion();
    std::cout << "Spinnaker library version: " << spinnakerLibraryVersion.major
              << "." << spinnakerLibraryVersion.minor << "."
              << spinnakerLibraryVersion.type << "."
              << spinnakerLibraryVersion.build << std::endl
              << std::endl;

    // Retrieve list of cameras from the system
    CameraList camList = system->GetCameras();
    const unsigned int numCameras = camList.GetSize();
    std::cout << "Number of cameras detected: " << numCameras << std::endl
              << std::endl;
    for (size_t i = 0; i < numCameras; i++) {
      auto pCam = camList.GetByIndex(i);
      pCams.push_back(pCam);
      pCam->Init();
      std::string serial = std::string(pCam->DeviceSerialNumber.ToString());
      serial2idx[serial] = i;
      pCam->DeInit();
    }
    camList.Clear();
  }

  ~SpinManager() { release(); }

  CameraPtr getCamera(int idx) {
    CameraPtr tmpCam = pCams.at(idx);
    return tmpCam;
  }
  CameraPtr getCamera(std::string serial) {
    int idx = serial2idx[serial];
    return getCamera(idx);
  }

  int size() { return pCams.size(); }

private:
  void release() {
    for (int i = 0; i < pCams.size(); i++) {
      pCams[i] = nullptr;
    }
    system->ReleaseInstance();
  }

  SystemPtr system;
  std::map<std::string, int> serial2idx;
  std::vector<CameraPtr> pCams;
};

class SpinCam {
public:
  SpinCam(CameraPtr pCam_) {
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

  ~SpinCam() {
    if (pCam) release();
  }

  bool read(cv::Mat& img_) {
    grab();
    bool ret = retrieve(img_);
    return ret;
  }

  void grab() {
    if (!isCapturing) {
      // Start capture
      displaySetting();
      pCam->BeginAcquisition();
      isCapturing = true;
    }

    if (isSoftwareTrigger) pCam->TriggerSoftware();
    isGrab = true;
  }

  bool retrieve(cv::Mat& img_) {
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

  void setFrameRate(double fps) {
    pCam->TriggerMode.SetValue(TriggerMode_Off);
    pCam->TriggerSource.SetValue(TriggerSource_Line0);
    setFrameRateAuto(false);
    std::cout << "Set framerate:" << fps << std::endl;
    pCam->AcquisitionFrameRate.SetValue(fps);
    isSoftwareTrigger = false;
  }

  void setROI(int offset_x, int offset_y, int width, int height) {
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

  void release() {
    if (isCapturing) pCam->EndAcquisition();
    pCam->DeInit();
    pCam = nullptr;
  }
  void setSoftwareTrigger() {
    std::cout << "Set softwareTrigger" << std::endl;
    pCam->TriggerMode.SetValue(TriggerMode_Off);
    pCam->TriggerSource.SetValue(TriggerSource_Software);
    pCam->TriggerMode.SetValue(TriggerMode_On);
    isSoftwareTrigger = true;
  }

  void displaySetting() {
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

  void setFrameRateAuto(bool flag) {
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
        std::cout
            << "Unable to set AcquisitionFrameRateAuto to OFF. Aborting..."
            << std::endl
            << std::endl;
        return;
      }
      // Set value
      value = ptrFrameRateAutoMode->GetValue();
    }
    ptrFrameRateAuto->SetIntValue(value);
  }

private:
  void setWhiteBalanceRatio(double val) {
    pCam->BalanceRatioSelector.SetValue(BalanceRatioSelector_Red);
    pCam->BalanceRatio.SetValue(val);
  }

  CameraPtr pCam;
  bool isGrab = false;
  bool isSoftwareTrigger = false;
  bool isCapturing = false;
  ImagePtr pConverted;  // save in memory
};

class SpinMultiCam {
public:
  void addCamera(CameraPtr& cam) {
    // Only software trigger is possible
    SpinCamPtr cam_ptr = std::make_shared<SpinCam>(cam);
    cam_ptr->setSoftwareTrigger();
    spincams.push_back(cam_ptr);
  }

  bool read(std::vector<cv::Mat>& imgs) {
    grab();
    bool ret = retrieve(imgs);
    return ret;
  }

  void grab() {
    for (auto& it : spincams) {
      it->grab();
    }
  }

  bool retrieve(std::vector<cv::Mat>& imgs) {
    imgs.clear();
    bool ret = true;
    for (auto& it : spincams) {
      cv::Mat tmpimg;
      ret &= it->retrieve(tmpimg);
      imgs.push_back(tmpimg);
    }
    return ret;
  }

  void release() {
    for (auto& it : spincams) {
      it->release();
    }
  }

private:
  std::vector<SpinCamPtr> spincams;
};

int main(int /*argc*/, char** /*argv*/) {
  SpinManager manager = SpinManager();

//#define STEREO
#ifdef STEREO
  SpinMultiCam cap;
  for (int i = 0; i < manager.size(); i++) {
    cap.addCamera(manager.getCamera(i));
  }
#else
  SpinCam cap(manager.getCamera(0));
  cap.setFrameRate(30);
  // cap.setFrameRateAuto(true);
#endif

  std::cout << "Capture start" << std::endl;
  int fps_cnt = 0;
  double diff_total = 0;
  while (true) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<cv::Mat> imgs;
    cv::Mat img, dst;

#ifdef STEREO
    if (cap.read(imgs)) {
#else
    if (cap.read(img)) {
      imgs.push_back(img);
#endif
      for (int i = 0; i < imgs.size(); i++) {
        cv::resize(imgs[i], dst, cv::Size(640, 640));
        std::ostringstream winname;
        winname << "img" << i;
        cv::imshow(winname.str(), dst);
      }

      int key = cv::waitKey(10);
      if (key == 27) break;
      if (key == 's') {
        cv::waitKey(0);
      }

      auto end = std::chrono::high_resolution_clock::now();
      auto diff =
          std::chrono::duration_cast<micro>(end - start).count() / 1000000.0;

      diff_total += diff;
      fps_cnt++;
      if (fps_cnt == 100) {
        diff = diff_total / fps_cnt;
        std::cout << "elps:" << diff * 1000 << "ms, fps:" << 1 / diff
                  << std::endl;
        diff_total = 0;
        fps_cnt = 0;
      }
    } else {
      std::cout << "Cap error" << std::endl;
    }
  }
  std::cout << "Capture end" << std::endl;

  cap.release();
  return 0;
}
