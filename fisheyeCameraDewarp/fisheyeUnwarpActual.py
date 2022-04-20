# https://medium.com/@kennethjiang/calibrate-fisheye-lens-using-opencv-part-2-13990f1b157f

import cv2
import numpy as np
import sys

# You should replace these 3 lines with the output in calibration step
#DIM=XXX
#K=np.array(YYY)
#D=np.array(ZZZ)
DIM=(2592, 1944)
K=np.array([[737.27915157973, 0.0, 1278.6197091932668], [0.0, 741.7129968681473, 987.9578976316317], [0.0, 0.0, 1.0]])
D=np.array([[0.09799660583508069], [-0.1404048797177384], [0.12545244890242996], [-0.03358188928891521]])

def undistort(img_path, balance=0.0, dim2=None, dim3=None):
    img = cv2.imread(img_path)
    undistortImg(img, balance, dim2, dim3)
def undistortImg(img, balance=0.0, dim2=None, dim3=None):
    dim1 = img.shape[:2][::-1]  #dim1 is the dimension of input image to un-distort
    assert dim1[0]/dim1[1] == DIM[0]/DIM[1], "Image to undistort needs to have same aspect ratio as the ones used in calibration"
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
    cv2.imshow("undistorted", undistorted_img)
    cv2.waitKey(0)
    cv2.destroyAllWindows()
if __name__ == '__main__':
    for p in sys.argv[1:]:
        undistort(p)
