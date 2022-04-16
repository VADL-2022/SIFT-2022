import sys
import os
p=os.path.join(os.path.dirname(os.path.realpath(__file__)),'..',)
sys.path.append(p)
import knn_matcher2

def run(img1Path, img2Path):
    knn_matcher2.mode = 1
    knn_matcher2.grabMode = 2
    knn_matcher2.shouldRunSkyDetection = False
    knn_matcher2.shouldRunUndistort = False
    knn_matcher2.skip = 0
    knn_matcher2.videoFilename = None
    knn_matcher2.showPreviewWindow = False
    knn_matcher2.imgs = [img1Path, img2Path]
    mat, w, h = knn_matcher2.run()
    return mat, w, h
