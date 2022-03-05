# https://forums.developer.nvidia.com/t/is-there-a-way-to-make-camera-undistort-cv2-fisheye-happen-faster/76522

import numpy as np
import cv2

def undistort_image(img, DIM, K, D, balance=0.0, dim2=None, dim3=None):
    dim1 = img.shape[:2][::-1]  #dim1 is the dimension of input image to un-distort
    if not dim2:
        dim2 = dim1
    if not dim3:
        dim3 = dim1
    scaled_K = K * dim1[0] / DIM[0]  # The values of K is to scale with image dimension.
    scaled_K[2][2] = 1.0  # Except that K[2][2] is always 1.0
    # This is how scaled_K, dim2 and balance are used to determine the final K used to un-distort image. OpenCV document failed to make this clear!
    new_K = cv2.fisheye.estimateNewCameraMatrixForUndistortRectify(scaled_K, D, dim2, np.eye(3), balance=balance)
    map1, map2 = cv2.fisheye.initUndistortRectifyMap(scaled_K, D, np.eye(3), new_K, dim3, cv2.CV_16SC2)
    undistorted_img = cv2.remap(img, map1, map2, interpolation=cv2.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT)
    return undistorted_img # this returns a np array

fullscale1FirstImage='/Volumes/MyTestVolume/Projects/VanderbiltRocketTeam/dataOutput/2022-02-28_20_22_58_CST/firstImage0.png'
def run():
    img=cv2.imread(fullscale1FirstImage)
    # https://medium.com/@kennethjiang/calibrate-fisheye-lens-using-opencv-333b05afa0b0
    # Need to find your values instead of these:
    DIM=(640, 480)
    K=np.array([[781.3524863867165, 0.0, 794.7118000552183], [0.0, 779.5071163774452, 561.3314451453386], [0.0, 0.0, 1.0]])
    D=np.array([[-0.042595202508066574], [0.031307765215775184], [-0.04104704724832258], [0.015343014605793324]])
    img2=undistort_image(img, DIM, K, D)
    windowName='undistort test'
    def on_change(val):
        K[0][0]=val
        img2=undistort_image(img, DIM, K, D)
        cv2.imshow(windowName,img2)
    cv2.imshow(windowName,img2)
    cv2.createTrackbar('slider', windowName, 0, 2000, on_change)
    if cv2.waitKey(0) & 0xFF == ord('q'): # https://stackoverflow.com/questions/20539497/python-opencv-waitkey0-does-not-respond
        exit(0)
run()
