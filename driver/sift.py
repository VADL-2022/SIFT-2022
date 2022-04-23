# import subprocess
# p = subprocess.Popen(["echo","aaaaaaaaaaaaaA"]) #["./sift_exe_release_commandLine","--main-mission", "--sift-params","-C_edge","2", "--sleep-before-running",)" + std::string(timeAfterMainDeployment) + R"("])
# # Above process runs asynchronously unless you wait:
# p.wait()

import videoCapture
import cv2
import numpy as np
import sys
import os
p=os.path.join(os.path.dirname(os.path.realpath(__file__)),'..')
sys.path.append(p)
import logging
import thread_with_exception
logging.basicConfig(level=logging.INFO)
import threading, queue
import signal
import knn_matcher2
import argparse
from datetime import datetime
import time

shouldStop=videoCapture.AtomicInt(0)
forceStop = False

class CustomVideoCapture: # Tries to implement cv2.VideoCapture's interface.
    def __init__(self, origVideoCap=None):
        self.q = queue.Queue()
        self.origVideoCap=origVideoCap
    def setOrigVideoCap(self,origVideoCap):
        self.origVideoCap=origVideoCap
    def read(self):
        if shouldStop.get() == 1 and (not finishRestAlways and self.q.qsize() < 1 # if queue is empty
                                      ):
            return (False # TODO: correct?
                    , None)
        return (True # TODO: correct?
                , self.q.get() # Blocks till frame ready
                )
    def get(self, attr):
        # if self.origVideoCap is None:
        #     print("self.origVideoCap is None")
        #     return 1
        while self.origVideoCap is None:
            print("self.origVideoCap is None")
            time.sleep(0.1)
        return self.origVideoCap.get(attr)
customVideoCapture = CustomVideoCapture()
finishRestAlways = True

def signal_handler(sig, frame):
    print('You pressed Ctrl+C! Stopping video capture thread...')
    #sys.exit(0)
    shouldStop.set(1)
    if forceStop:
        exit(0)
    # NOTE: this does finish rest always by default!
    customVideoCapture.q.put(None) # placeholder for stopping

# Function to run on the video capture thread
def videoCaptureThreadFunction(name, **kwargs):
    # print(kwargs)
    # input()
    global shouldStop

    # Check if video capture is already running
    if shouldStop.get() != 0:
        print("videoCaptureThreadFunction(): Video capture is already running, not starting again")
        return

    # Start the actual video capture
    logging.info("Thread %s: starting", name)
    videoCapture.run(shouldStop, **kwargs)
    logging.info("Thread %s: finishing", name)

# Starts the video capture
def startVideoCapture(name, **kwargs):
    global videoCaptureThread

    # Check if a video capture thread is already running
    if shouldStop.get() != 0:
        print("startVideoCapture(): Video capture is already running, not starting again")
        return

    # Start the video capture thread
    videoCaptureThread = thread_with_exception.thread_with_exception(name=name, target=videoCaptureThreadFunction, args=(name,), kwargs_=kwargs)
    videoCaptureThread.start()

def onSetVideoCapture(cap):
    customVideoCapture.setOrigVideoCap(cap)
def onFrame(frame):
    customVideoCapture.q.put(frame)

# https://docs.python.org/3/library/argparse.html
parser = argparse.ArgumentParser(description='The bad boy SIFT but implemented in python..')
parser.add_argument('--skip', type=int, nargs='+', default=0,
                    help="initial skip")
parser.add_argument('--show-preview-window', action='store_true', default=False,
                    help='whether to show the preview window')
parser.add_argument('--frameskip', type=int, nargs='+', default=1,
                    help="skip this number of frames after every frame processed")
# --save-first-image --video-file-data-source --video-file-data-source-path Data/fullscale1/Derived/SIFT/output.mp4 --no-sky-detection
parser.add_argument('--save-first-image', action='store_true', default=True,
                    help='whether to save the first image')
parser.add_argument('--video-file-data-source', action='store_true', default=False,
                    help='')
parser.add_argument('--video-file-data-source-path', type=str, nargs='+', default=None,
                    help="")
parser.add_argument('--no-sky-detection', action='store_true', default=False,
                    help='')
videoFilename = None
# https://stackoverflow.com/questions/12834785/having-options-in-argparse-with-a-dash
namespace=parser.parse_args() #vars(parser.parse_args()) # default is from argv but can provide a list here
print(namespace)
showPreviewWindow=namespace.show_preview_window
skip=namespace.skip
frameSkip=namespace.frameskip[0] if isinstance(namespace.frameskip, list) else namespace.frameskip # HACK
shouldRunSkyDetection=not namespace.no_sky_detection
videoFileDataSourcePath=namespace.video_file_data_source_path[0] if isinstance(namespace.video_file_data_source_path, list) else namespace.video_file_data_source_path # HACK
videoFileDataSource = namespace.video_file_data_source
if videoFileDataSource:
    forceStop=True
def runOnTheWayDown(capAPI, pSave):
    knn_matcher2.mode = 1
    knn_matcher2.grabMode = 1
    knn_matcher2.shouldRunSkyDetection = shouldRunSkyDetection
    knn_matcher2.shouldRunUndistort = True #False
    knn_matcher2.skip = skip
    knn_matcher2.videoFilename = None
    knn_matcher2.showPreviewWindow = showPreviewWindow
    knn_matcher2.reader = capAPI #capAPI if not videoFileDataSource else cv2.VideoCapture(videoFileDataSourcePath)
    knn_matcher2.frameSkip = frameSkip #40#1#5#10#20
    knn_matcher2.waitAmountStandard = 1 # (Only for showPreviewWindow == True)

    # knn_matcher2.nfeatures = 0
    # knn_matcher2.nOctaveLayers = 9
    # knn_matcher2.contrastThreshold = 0.03
    # knn_matcher2.edgeThreshold = 10
    # knn_matcher2.sigma = 0.8
    
    knn_matcher2.nfeatures = 0
    knn_matcher2.nOctaveLayers = 10
    knn_matcher2.contrastThreshold = 0.02
    knn_matcher2.edgeThreshold = 10
    knn_matcher2.sigma = 0.8
    knn_matcher2.sift = cv2.xfeatures2d.SIFT_create(knn_matcher2.nfeatures, knn_matcher2.nOctaveLayers, knn_matcher2.contrastThreshold, knn_matcher2.edgeThreshold, knn_matcher2.sigma)
    
    rets = knn_matcher2.run(pSave)
    return rets

if __name__ == '__main__':
    name = "SIFT_Cam"
    signal.signal(signal.SIGINT, signal_handler)
    
    now = datetime.now() # current date and time
    date_time = now.strftime("%m_%d_%Y_%H_%M_%S")
    o1=now.strftime("%Y-%m-%d_%H_%M_%S_%Z")
    outputFolderPath=os.path.join('.', 'dataOutput', o1)
    pSave = outputFolderPath
    
    try:
        # NOTE: first image given should match what the sky detector chooses so we re-set firstImage and firstImageFilename here
        if videoFileDataSource:
            capOrig=cv2.VideoCapture(videoFileDataSourcePath)
            startVideoCapture(name, capOrig=capOrig, onFrame=onFrame, fps=capOrig.get(cv2.CAP_PROP_FPS), onSetVideoCapture=onSetVideoCapture, outputFolderPath=pSave)
        else:
            startVideoCapture(name, onFrame=onFrame, fps=5, onSetVideoCapture=onSetVideoCapture, outputFolderPath=pSave)
        
        accMat, w, h, firstImage, firstImageOrig, firstImageFilename = runOnTheWayDown(customVideoCapture
            #cv2.VideoCapture(videoFilename) if videoFileDataSource else customVideoCapture
            , pSave)
    except knn_matcher2.EarlyExitException as e:
        accMat = e.acc
        w=e.w
        h=e.h
        firstImage=e.firstImage
        firstImageFilename=e.firstImageFilename
