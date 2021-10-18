import cv2
import os

devs = os.listdir('/dev')
vid_indices = [int(dev[-1]) for dev in devs 
               if dev.startswith('video')]
vid_indices = sorted(vid_indices)
print(vid_indices)

def returnCameraIndexes():
    # checks the first 10 indexes.
    index = 0
    arr = []
    i = 10
    while i > 0:
        cap = cv2.VideoCapture(index)
        if cap.read()[0]:
            arr.append(index)
            cap.release()
        index += 1
        i -= 1
    return arr

print(returnCameraIndexes())
