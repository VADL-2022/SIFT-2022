import sys
import os
p=os.path.join(os.path.dirname(os.path.realpath(__file__)),'..',)
sys.path.append(p)
import knn_matcher2
import cv2

def run(img1PathOrImage, img2PathOrImage, showPreviewWindow=False):
    knn_matcher2.mode = 1
    knn_matcher2.grabMode = 2
    knn_matcher2.shouldRunSkyDetection = False
    knn_matcher2.shouldRunUndistort = False
    knn_matcher2.skip = 0
    knn_matcher2.videoFilename = None
    knn_matcher2.showPreviewWindow = showPreviewWindow
    knn_matcher2.imgs = [img1PathOrImage, img2PathOrImage]
    knn_matcher2.frameSkip = 1

    # Crank up the settings -- takes longer but better result
    knn_matcher2.nfeatures = 0
    knn_matcher2.nOctaveLayers = 10
    knn_matcher2.contrastThreshold = 0.02
    knn_matcher2.edgeThreshold = 10
    knn_matcher2.sigma = 0.8
    knn_matcher2.sift = cv2.xfeatures2d.SIFT_create(knn_matcher2.nfeatures, knn_matcher2.nOctaveLayers, knn_matcher2.contrastThreshold, knn_matcher2.edgeThreshold, knn_matcher2.sigma)
    
    rets = knn_matcher2.run()
    return rets
