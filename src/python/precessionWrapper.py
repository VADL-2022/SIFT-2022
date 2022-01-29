import Precession

prevOrientation = None
def judgeImageWrapper(imuYPR):
    if prevOrientation is None:
        prevOrientation = Precession.init_orientation(imuYPR)
    shouldKeepImage, prevOrientation = Precession.judge_image(prevOrientation, imuYPR)
    return shouldKeepImage
    
