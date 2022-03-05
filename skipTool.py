import numpy as np
import cv2
import os
import shutil

def run():
    import subprocess
    path="Data/fullscale1/Derived/SIFT/ExtractedFrames_unrotated"
    p=subprocess.run(["compareNatSort/compareNatSort", path, ".png"], capture_output=True)
    imgs=p.stdout.split(b'\n')
    angle = np.rad2deg(np.pi/6)
    print("Output is image followed by its angle used:")
    for imgName in imgs:
        imgName=imgName.decode('utf-8')
        print(imgName)
        img=cv2.imread(imgName)
        #print(img)
        img2=img

        specific_point = np.array(img.shape[:2][::-1])/2
        windowName='undistort test'
        def on_change(val):
            nonlocal img2
            nonlocal angle
            #angle=val
            #img2=rotate(img, np.deg2rad(angle), specific_point)
            cv2.imshow(windowName,img2)
        on_change(angle)
        #cv2.createTrackbar('slider', windowName, 0, 360, on_change)
        key=cv2.waitKey(0)
        if key & 0xFF == ord('q'): # https://stackoverflow.com/questions/20539497/python-opencv-waitkey0-does-not-respond
            exit(0)
        elif key & 0xFF == ord('w'): # Move image (count as discarded/"skip")
            # Move image
            shutil.move(imgName, os.path.join(path, 'Discarded'))
            # Go to next image
            print(angle)
            continue
        elif key & 0xFF == ord('s'): # Don't move
            # Go to next image
            continue
run()
