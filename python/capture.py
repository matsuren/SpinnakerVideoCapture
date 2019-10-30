import argparse
import time

import cv2

from PySpinCap import PySpinManager


def main():
    # arg
    parser = argparse.ArgumentParser(description='Spinnaker capture')
    parser.add_argument('-n', '--num', nargs='+', type=int, default=[0],
                        help='Camera number for capture. It can be list of number')
    args = parser.parse_args()
    print('camera id:{}'.format(args.num))
    STEREO = True if len(args.num) > 1 else False
    if STEREO:
        print('##\n##Stereo mode: software synchronization is enabled!\n##')

    # start manager
    manager = PySpinManager()
    if len(manager.cams) == 0:
        print('No camera detected')
        return

    if STEREO:
        cap = manager.get_multi_camera(args.num)
    else:
        cap = manager.get_camera(args.num[0])

    print('#####################')
    print('### start capturing')
    elps = []
    save_imgs = []
    while True:
        start = time.time()

        if STEREO:
            ret, imgs = cap.read()
        else:
            ret, img = cap.read()
            imgs = [img]

        if not ret:
            print('capture error')
            continue

        for i, img in enumerate(imgs):
            img = cv2.resize(img, (640, 640))
            cv2.imshow('img{}'.format(i), img)
        key = cv2.waitKey(20)
        elps.append((time.time() - start))

        if key == ord('s'):
            cv2.waitKey()
            save_imgs.append(imgs)
        elif key == 27:
            break

        if len(elps) == 100:
            print('- FPS:{}'.format(len(elps) / sum(elps)))
            elps = []

    cv2.destroyAllWindows()
    print('### finish capturing')
    print('#####################')

    cap.release()


if __name__ == "__main__":
    main()
