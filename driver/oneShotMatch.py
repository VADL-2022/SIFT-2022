import sys
import os
p=os.path.join(os.path.dirname(os.path.realpath(__file__)),'..',)
sys.path.append(p)
import knn_matcher2

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
    rets = knn_matcher2.run()
    return rets
