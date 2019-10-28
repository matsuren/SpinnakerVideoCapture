import PySpin
import cv2
import numpy as np


# Single camera class
class PySpinCam(object):
    def __init__(self, cam):
        if cam is None:
            print('cam is None. Already used in other object')
            return
        self.cam = cam
        self.cam.Init()
        self.cam.TLStream.StreamBufferHandlingMode.SetValue(PySpin.StreamBufferHandlingMode_NewestOnly)
        self.cam.PixelFormat.SetValue(PySpin.PixelFormat_BayerGB8)
        print('Pixel format:', self.cam.PixelFormat.GetCurrentEntry().GetSymbolic())
        device_model = self.cam.DeviceModelName.ToString()
        print('Device model:', device_model)
        # Get serial number
        print('Serial number:', self.cam.DeviceSerialNumber.ToString())
        if device_model == "Grasshopper3 GS3-U3-41C6C":
            print("Set ROI since Model is ", device_model)
            self.set_roi(224, 224, 1600, 1600)
            # self.set_roi(0, 0, 2048, 2048)

        # image acquisition
        self.cam.AcquisitionMode.SetValue(PySpin.AcquisitionMode_Continuous)

        # white balance setting
        self.set_white_balance_ratio(1.18, select='Red')
        self.set_white_balance_ratio(1.46, select='Blue')

        #  start capturing
        self.cam.BeginAcquisition()
        self.isGrab = False
        self.isSoftwareTrigger = False

    def read(self):
        self.grab()
        ret, img = self.retrieve()
        return ret, img

    def grab(self):
        if self.isSoftwareTrigger:
            self.cam.TriggerSoftware()
        self.isGrab = True

    def retrieve(self):
        if not self.isGrab and self.isSoftwareTrigger:
            print('Call grab first')
            return False, np.array([])

        image_result = self.cam.GetNextImage()
        if image_result.IsIncomplete():
            return False, np.array([])

        # Bayer to color
        img = image_result.GetNDArray()
        #     image_converted = image_result.Convert(PySpin.PixelFormat_BGR8, PySpin.HQ_LINEAR) # slow
        #     img = image_converted.GetNDArray()
        img = cv2.cvtColor(img, cv2.COLOR_BAYER_GR2BGR)  # bayer pattern is correct?
        image_result.Release()
        self.isGrab = False
        return True, img

    def release(self):
        if self.cam is None:
            return

        self.cam.EndAcquisition()
        self.cam.DeInit()
        self.cam = None

    def __del__(self):
        self.release()

    ##########################
    ## setting
    def set_software_trigger(self):
        print('Set software trigger')
        self.cam.TriggerMode.SetValue(PySpin.TriggerMode_Off)
        self.cam.TriggerSource.SetValue(PySpin.TriggerSource_Software)
        self.cam.TriggerMode.SetValue(PySpin.TriggerMode_On)
        self.isSoftwareTrigger = True

    def set_white_balance_ratio(self, val, select='Red'):
        if select == 'Red':
            self.cam.BalanceRatioSelector.SetValue(PySpin.BalanceRatioSelector_Red)
        elif select == 'Blue':
            self.cam.BalanceRatioSelector.SetValue(PySpin.BalanceRatioSelector_Blue)
        self.cam.BalanceRatio.SetValue(val)

    def set_frame_rate(self, fps):
        print('Set FPS:', fps)
        nodemap = self.cam.GetNodeMap()
        self.cam.TriggerMode.SetValue(PySpin.TriggerMode_Off)
        node = PySpin.CEnumerationPtr(nodemap.GetNode('AcquisitionFrameRateAuto'))
        if PySpin.IsAvailable(node) and PySpin.IsWritable(node):
            val = PySpin.CEnumEntryPtr(node.GetEntryByName("Off")).GetValue()  # Once, Continuous
            node.SetIntValue(val)
            self.cam.AcquisitionFrameRate.SetValue(fps)
            self.isSoftwareTrigger = False
        else:
            print('Error setting fps:')

    def set_roi(self, offset_x, offset_y, width, height):
        print('Set ROI:{}, {}, {}, {}'.format(offset_x, offset_y, width, height))
        if self.cam.OffsetX.GetValue() < offset_x:
            self.cam.Width.SetValue(int(width / 32) * 32)
            self.cam.Height.SetValue(int(height / 2) * 2)
            self.cam.OffsetX.SetValue(offset_x)
            self.cam.OffsetY.SetValue(offset_y)
        else:
            self.cam.OffsetX.SetValue(offset_x)
            self.cam.OffsetY.SetValue(offset_y)
            self.cam.Width.SetValue(int(width / 32) * 32)
            self.cam.Height.SetValue(int(height / 2) * 2)


# Multiple camera class (software sync)
class PySpinMultiCam(object):
    def __init__(self):
        self.spincams = []

    def add_camera(self, spincam):
        self.spincams.append(spincam)

    def read(self):
        self.grab()
        ret, imgs = self.retrieve()
        return ret, imgs

    def grab(self):
        for it in self.spincams:
            it.grab()

    def retrieve(self):
        ret = True
        imgs = []
        for it in self.spincams:
            flag, img = it.retrieve()
            ret &= flag
            imgs.append(img)
        return ret, imgs

    def release(self):
        for it in self.spincams:
            it.release()

    def __del__(self):
        self.release()


if __name__ == "__main__":
    from PySpinCap import PySpinManager

    manager = PySpinManager()

    cap = manager.get_camera(0)
    ret, img = cap.read()
    print(img[:2, :2])
    cap.release()
    print('Finish')
