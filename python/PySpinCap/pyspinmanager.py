import PySpin

from .pyspincam import PySpinCam, PySpinMultiCam


# Manager class
class PySpinManager(object):
    def __init__(self):
        # Retrieve singleton reference to system object
        self.system = PySpin.System.GetInstance()
        # Get current library version
        version = self.system.GetLibraryVersion()
        print('Library version: %d.%d.%d.%d' % (version.major, version.minor, version.type, version.build))

        # Get camera list
        cam_list = self.system.GetCameras()
        num_cameras = cam_list.GetSize()
        print('Number of cameras detected: %d' % num_cameras)
        self.cams = []
        self.serial2idx = {}
        for i in range(num_cameras):
            self.cams.append(cam_list.GetByIndex(i))
            self.cams[-1].Init()
            serial = self.cams[-1].DeviceSerialNumber.ToString()
            self.serial2idx[serial] = i
            self.cams[-1].DeInit()

        cam_list.Clear()

    def get_camera(self, idx):
        if isinstance(idx, str):
            idx = self.serial2idx[idx]
        elif isinstance(idx, int):
            pass
        else:
            raise PySpin.SpinnakerException('idx should be serial(str) or index(int)')

        cam = self.cams[idx]
        self.cams[idx] = None
        spincam = PySpinCam(cam)
        spincam.set_frame_rate(30)
        return spincam

    def get_multi_camera(self, idx):
        if not isinstance(idx, list):
            raise PySpin.SpinnakerException('idx should be list of index or serial')
        multi_cap = PySpinMultiCam()
        for it in idx:
            spincam = self.get_camera(it)
            spincam.set_software_trigger()
            multi_cap.add_camera(spincam)
        return multi_cap

    def release(self):
        # Release cameras
        for i in range(len(self.cams)):
            self.cams[i] = None

        # Release system instance
        self.system.ReleaseInstance()
        self.system = None
        print('release everything')

    def __del__(self):
        self.release()
