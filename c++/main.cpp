#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

class SpinCam;
using SpinCamPtr = std::shared_ptr<SpinCam>;

class SpinManager {
public:
    SpinManager() {
        system = System::GetInstance();
        const LibraryVersion spinnakerLibraryVersion = system->GetLibraryVersion();
        std::cout << "Spinnaker library version: "
                  << spinnakerLibraryVersion.major << "."
                  << spinnakerLibraryVersion.minor << "."
                  << spinnakerLibraryVersion.type << "."
                  << spinnakerLibraryVersion.build << std::endl
                  << std::endl;

        // Retrieve list of cameras from the system
        CameraList camList = system->GetCameras();
        const unsigned int numCameras = camList.GetSize();
        cout << "Number of cameras detected: " << numCameras << endl
             << endl;
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

    ~SpinManager() {
        release();
    }

    CameraPtr getCamera(int idx) {
        CameraPtr tmpCam = pCams.at(idx);
        return tmpCam;
    }
    CameraPtr getCamera(std::string serial) {
        int idx = serial2idx[serial];
        return getCamera(idx);
    }

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
        pCam = pCam_;
        pCam->Init();
        pCam->PixelFormat.SetValue(PixelFormat_BayerGB8);
        std::cout << "Pixel format:" << pCam->PixelFormat.ToString() << std::endl;
        std::cout << "Device model:" << pCam->DeviceModelName.ToString() << std::endl;
        std::cout << "Serial number:" << pCam->DeviceSerialNumber.ToString() << std::endl;

        // Image acquisition
        pCam->AcquisitionMode.SetValue(AcquisitionMode_Continuous);
        // Software trigger
        setSoftwareTrigger();
        // White balance
        setWhiteBalanceRatio(1.06);

        // Start capture
        pCam->BeginAcquisition();
        isGrab = false;
    }

    void grab() {
        pCam->TriggerSoftware();
        isGrab = true;
    }

    bool retrieve(cv::Mat& img_) {
        ImagePtr pResultImage = pCam->GetNextImage();
        if (pResultImage->IsIncomplete()) {
            return false;
        }

        if (true) {
            int width = pResultImage->GetWidth();
            int height = pResultImage->GetHeight();
            cv::Mat img(height, width, CV_8UC1, pResultImage->GetData());
            cv::cvtColor(img, img_, cv::COLOR_BayerGR2BGR);
        }
        else {
            pConverted = pResultImage->Convert(PixelFormat_BGR8, HQ_LINEAR);
            int width = pConverted->GetWidth();
            int height = pConverted->GetHeight();
            img_ = cv::Mat(height, width, CV_8UC3, pConverted->GetData());
        }

        pResultImage->Release();
        return true;
    }

    bool read(cv::Mat& img_) {
        grab();
        bool ret = retrieve(img_);
        return ret;
    }

    void release() {
        pCam->EndAcquisition();
        pCam->DeInit();
        pCam = nullptr;
    }

private:
    void setSoftwareTrigger() {
        pCam->TriggerMode.SetValue(TriggerMode_Off);
        pCam->TriggerSource.SetValue(TriggerSource_Software);
        pCam->TriggerMode.SetValue(TriggerMode_On);
    }
    void setWhiteBalanceRatio(double val) {
        pCam->BalanceRatioSelector.SetValue(BalanceRatioSelector_Red);
        pCam->BalanceRatio.SetValue(val);
    }

    CameraPtr pCam;
    bool isGrab;
    ImagePtr pConverted; // save in memory
};

int main(int /*argc*/, char** /*argv*/) {
    SpinManager manager = SpinManager();
    SpinCam cap(manager.getCamera(0));

    std::cout << "Capture start" << std::endl;

    int fps_cnt = 0;
    double diff_total = 0;
    //cap.grab();
    while (true) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat img, dst;
        //if(cap.retrieve(img)){
        //  cap.grab();
        if (cap.read(img)) {
            cv::resize(img, dst, cv::Size(640, 640));
            cv::imshow("test", dst);
            int key = cv::waitKey(10);
            if (key == 27)
                break;

            auto end = std::chrono::high_resolution_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000.0;
            diff_total += diff;
            fps_cnt++;
            if (fps_cnt == 50) {
                diff = diff_total / fps_cnt;
                std::cout << "elps:" << diff << ", fps:" << 1 / diff << std::endl;
                diff_total = 0;
                fps_cnt = 0;
            }
        }
        else {
            std::cout << "Cap error" << std::endl;
        }
    }
    std::cout << "Capture end" << std::endl;

    cap.release();
    return 0;
}
