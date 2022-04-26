import knn_matcher2
import cv2
import numpy as np
import sys
import os
p=os.path.join(os.path.dirname(os.path.realpath(__file__)),'driver')
sys.path.append(p)
import driver.sift as sift

def sift2Landing():
    knn_matcher2.waitAmountStandard=0
    res=knn_matcher2.showLandingPos(cv2.imread('Data/nasa/sift2/2022-04-23_13_43_21_/firstImage0.png'), np.matrix([[-8.54171994e-02, -3.12911562e-02,  1.22611625e+02],[ 7.24061720e-02,  2.65255842e-02,  3.54256794e+02],[ 0.00000000e+00,  0.00000000e+00,  1.00000000e+00]]))
#print(res)

def sift1Landing():
    knn_matcher2.waitAmountStandard=0
    res=knn_matcher2.showLandingPos(cv2.imread('Data/nasa/sift1/2022-04-23_13_43_26_/firstImage0.png'), np.matrix([[1.1577159661510255, 0.89734101924389, 41.55299586383646],
                                                                                                               [-1.519727186706962, -0.6057636954943331, 652.6997095839926],
                                                                                                               [0.0, 0.0, 1.0]]))

def sift2Descent(frameskip=1, undistort=True):
    class CustomNamespace:
        pass
    sift.customArgsNamespace = CustomNamespace()
    sift.customArgsNamespace.show_preview_window = True
    sift.customArgsNamespace.skip = 0
    sift.customArgsNamespace.frameskip = frameskip
    sift.customArgsNamespace.no_sky_detection = False
    sift.customArgsNamespace.video_file_data_source = True
    sift.customArgsNamespace.video_file_data_source_path = 'Data/nasa/sift1/2022-04-23_13_43_26_/output.mp4'
    sift.customArgsNamespace.no_undistort = not undistort
    sift.run()

sift2Descent(frameskip=5, undistort=False)
