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

shouldStop=videoCapture.AtomicInt(0)
customVideoCapture = CustomVideoCapture()

def signal_handler(sig, frame):
    print('You pressed Ctrl+C! Stopping video capture thread...')
    #sys.exit(0)
    shouldStop.set(1)
    customVideoCapture.put(None) # placeholder for stopping

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

class CustomVideoCapture: # Tries to implement cv2.VideoCapture's interface.
    def __init__(self, origVideoCap=None):
        self.q = queue.Queue()
        self.origVideoCap=origVideoCap
    def setOrigVideoCap(self,origVideoCap):
        self.origVideoCap=origVideoCap
    def read(self):
        if shouldStop.get() == 1:
            return (False # TODO: correct?
                    , None)
        return (True # TODO: correct?
                , self.q.get() # Blocks till frame ready
                )
    def get(self, attr):
        if self.origVideoCap is None:
            print("self.origVideoCap is None")
            return 1
        return self.origVideoCap.get(attr)
def onSetVideoCapture(cap):
    customVideoCapture.setOrigVideoCap(cap)
def onFrame(frame):
    customVideoCapture.q.put(frame)

# https://docs.python.org/3/library/argparse.html
parser = argparse.ArgumentParser(description='The bad boy SIFT but implemented in python..')
parser.add_argument('--skip', metavar='N', type=int, nargs='+', default=0,
                    help="initial skip")
parser.add_argument('--show-preview-window', action='store_true', default=False,
                    help='whether to show the preview window')
parser.add_argument('--frameskip', metavar='N', type=int, nargs='+', default=1,
                    help="skip this number of frames after every frame processed")
videoFileDataSource = False
videoFilename = None
# https://stackoverflow.com/questions/12834785/having-options-in-argparse-with-a-dash
namespace=parser.parse_args() #vars(parser.parse_args()) # default is from argv but can provide a list here
print(namespace)
showPreviewWindow=namespace.show_preview_window
skip=namespace.skip
frameSkip=namespace.frameskip
def runOnTheWayDown(capAPI):
    knn_matcher2.mode = 1
    knn_matcher2.grabMode = 1
    knn_matcher2.shouldRunSkyDetection = True
    knn_matcher2.shouldRunUndistort = True #False
    knn_matcher2.skip = skip
    knn_matcher2.videoFilename = None
    knn_matcher2.showPreviewWindow = showPreviewWindow
    knn_matcher2.reader = capAPI
    knn_matcher2.frameSkip = frameSkip #40#1#5#10#20
    rets = knn_matcher2.run()
    return rets

if __name__ == '__main__':
    name = "SIFT_Cam"
    signal.signal(signal.SIGINT, signal_handler)
    startVideoCapture(name, onFrame=onFrame, fps=5, onSetVideoCapture=onSetVideoCapture)
    
    try:
        # NOTE: first image given should match what the sky detector chooses so we re-set firstImage and firstImageFilename here
        accMat, w, h, firstImage, firstImageOrig, firstImageFilename = runOnTheWayDown(cv2.VideoCapture(videoFilename) if videoFileDataSource else customVideoCapture)
    except knn_matcher2.EarlyExitException as e:
        accMat = e.acc
        w=e.w
        h=e.h
        firstImage=e.firstImage
        firstImageFilename=e.firstImageFilename
