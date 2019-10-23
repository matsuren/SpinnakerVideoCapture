#include <chrono>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <sstream>
#include <string>

#include "spincamera.hpp"
#include "spinmanager.hpp"

class SpinCam;
using SpinCamPtr = std::shared_ptr<SpinCam>;
using micro = std::chrono::microseconds;

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
