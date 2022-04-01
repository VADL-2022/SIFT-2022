import sys
import os
p=os.path.join(os.path.dirname(os.path.realpath(__file__)),'cnn-registration','src')
sys.path.append(p)
os.chdir(p)

import demo
# import importlib
# cnn_registration_demo = importlib.import_module("cnn-registration.src.demo")

# Simple test: fullscale1 with simple translation #
#IX_path = '../img/1a.jpg'
IX_path = '../img/Screen Shot 2022-03-29 at 2.12.34 PM.png' # bragg farm: '/Volumes/MyTestVolume/SeniorSemester1_Vanderbilt_University/RocketTeam/MyWorkspaceAndTempFiles/FromEthan/BF_No_Grid_640width.jpg'
#IY_path = '../img/1b.jpg'
IY_path = '../img/Screen Shot 2022-03-29 at 2.13.17 PM.png' #'../img/1b.jpg' # 36.77681 degrees N, 87.51291 degrees W launch. landing: 36.77541 degrees north, 87.51351 degrees W.
# All possible areas that the rocket could land for fullscale1 are in the screenshots in the ../img folder.
# #
# Advanced: fullscale1 #
# high up: IX_path = '../../Data/fullscale1/Derived/SIFT/ExtractedFrames/thumb0007.png'
IX_path = '../../Data/fullscale1/Derived/SIFT/ExtractedFrames/thumb0106.png'
IY_path = 'cnn-registration/img/Screen Shot 2022-03-29 at 2.13.17 PM.png'
# #
# Undistorted with ROI: fullscale1 #
IY_path = '../../undistortedImages/thumb0089.undistorted.png'    # Doesn't exist yet: '/Volumes/MyTestVolume/Projects/VanderbiltRocketTeam/Data/fullscale1/Derived/SIFT/ExtractedFrames_undistorted_withROI/thumb0106.png'
IX_path = '../../satelliteImages/Screen Shot 2022-03-29 at 2.13.17 PM.png'
# #
# Undistorted with ROI: fullscale1 #
# IY_path = '../../Data/fullscale1/Derived/SIFT/ExtractedFrames/thumb0127.png'
# IX_path = '../img/Screen Shot 2022-03-29 at 2.13.17 PM.png'
# #

demo.run(IX_path, IY_path)
