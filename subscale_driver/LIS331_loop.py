# -*- coding: utf-8 -*-
"""
Created on Thu Jan 27 17:43:47 2022

@author: kdmen

YOU MUST DOWNLOAD SMSBUS ON THE RPI BEFORE YOU CAN RUN THIS:
    
DATA SHEET FOR THIS IMU
https://www.sparkfun.com/datasheets/Sensors/Accelerometer/LIS331HH.pdf

"""

import smbus
import time
from csv import writer
import time
import logging
import threading
import thread_with_exception
import sys
import numpy as np
from datetime import datetime
import RPi.GPIO as GPIO

logOnly = (sys.argv[1] == '1') if len(sys.argv) > 1 else False # If set to "1", don't do anything but log IMU data
takeoffGs = float(sys.argv[2]) if len(sys.argv) > 2 else None
if not logOnly and takeoffGs is None:
    print("Must provide takeoff g's")
    exit(1)
switchCamerasTime = float(sys.argv[3]) if len(sys.argv) > 3 else None
if not logOnly and switchCamerasTime is None:
    print("Must provide switchCamerasTime in milliseconds")
    exit(1)
takeoffTime = None
calibrationFile = (sys.argv[3]) if len(sys.argv) > 3 else None
if calibrationFile is None:
    print("Must provide calibrationFile")
    exit(1)

if not logOnly:
    def thread_function(name):
        logging.info("Thread %s: starting", name)
        import videoCapture # This is all that is needed, it will run from this.
        logging.info("Thread %s: finishing", name)
    name="videoCapture"
    videoCaptureThread = thread_with_exception.thread_with_exception(name=name, target=thread_function, args=(name,))
    videoCaptureThread.start()
else:
    videoCaptureThread = None
    name=None

def append_list_as_row(file_name, list_of_elem):
    # Open file in append mode
    with open(file_name, 'a+', newline='') as write_obj:
        # Create a writer object from csv module
        csv_writer = writer(write_obj)
        # Add contents of list as last row in the csv file
        csv_writer.writerow(list_of_elem)

timestr = time.strftime("%Y%m%d-%H%M%S")
my_log = "dataOutput/LOG_" + timestr + ".LIS331.csv"

# Create a new (empty) csv file
with open(my_log, 'w', newline='') as file:
    new_file = writer(file)

# Get I2C bus
bus = smbus.SMBus(1)

#Syntax:
#write_byte_data(self, addr, cmd, val)

# SET THE DATA RATE
###############################################################################
# H3LIS331DL address, 0x18(24)
# Select control register 1, 0x20(32)
#		0x27(39)	Power ON mode, Data output rate = 50 Hz
#					X, Y, Z-Axis enabled
address=0x19
bus.write_byte_data(address, 0x20, 0x27)
# Required Binary: 001 (normal power mode), 00 (50 Hz), 111 (enable XYZ)
# Equivalent Hex: 00100111 -> 0x27
# TOGGLE THE MAXIMUM RANGE
###############################################################################
# H3LIS331DL address, 0x18(24)
# Select control register 4, 0x23(35)
#		0x00(00)	Continuous update,
#Full scale selection = +/-24g: 0x18
#SUBSCALE = +/- 12g: 0x10
#bus.write_byte_data(0x18, 0x23, 0x00)
bus.write_byte_data(address, 0x23, 0x10)

time.sleep(0.5)

avgX=0
avgY=0
avgZ=0
counter=0

offsetX=0
offsetY=0
offsetZ=0

def calibrate(file):
    global offsetX
    global offsetY
    global offsetZ
    with open(file, 'r') as f:
        for line in f:
            pass
        last_line = line
        offsetX,offsetY,offsetZ=list(map(float, last_line.split(",")[1:]))

def runOneIter():
    global avgX
    global avgY
    global avgZ
    global counter
    
    '''Do I ever need to sleep here? Or will the while take care of it?'''
    
    # X AXIS
    ###############################################################################
    # H3LIS331DL address, 0x18(24)
    # Read data back from 0x28(40), 2 bytes
    # X-Axis LSB, X-Axis MSB
    data0 = bus.read_byte_data(address, 0x28)
    data1 = bus.read_byte_data(address, 0x29)
    
    # Convert the data
    xAccl = data1 * 256 + data0
    if xAccl > 32767 :
    	xAccl -= 65536
    
    
    # Y AXIS
    ###############################################################################
    # H3LIS331DL address, 0x18(24)
    # Read data back from 0x2A(42), 2 bytes
    # Y-Axis LSB, Y-Axis MSB
    data0 = bus.read_byte_data(address, 0x2A)
    data1 = bus.read_byte_data(address, 0x2B)
    
    # Convert the data
    yAccl = data1 * 256 + data0
    if yAccl > 32767 :
    	yAccl -= 65536
    
    
    # Z AXIS
    ###############################################################################
    # H3LIS331DL address, 0x18(24)
    # Read data back from 0x2C(44), 2 bytes
    # Z-Axis LSB, Z-Axis MSB
    data0 = bus.read_byte_data(address, 0x2C)
    data1 = bus.read_byte_data(address, 0x2D)
    
    # Convert the data
    zAccl = data1 * 256 + data0
    if zAccl > 32767 :
    	zAccl -= 65536
    
    # LOG TO CSV
    ###############################################################################
    # Output data to screen
    #print("Acceleration in X-Axis : %d" %xAccl)
    #print("Acceleration in Y-Axis : %d" %yAccl)
    #print("Acceleration in Z-Axis : %d" %zAccl)

    avgX += xAccl
    avgY += yAccl
    avgZ += zAccl
    counter += 1
    my_accels = [(xAccl-offsetX)/1000.0*9.81, (yAccl-offsetY)/1000.0*9.81, (zAccl-offsetZ)/1000.0*9.81]
    append_list_as_row(my_log, my_accels)

    # Wait for takeoff
    if not logOnly:
        magnitude = np.linalg.norm(np.array(my_accels))
        if magnitude > takeoffGs*9.81 and takeoffTime is None:
            takeoffTime = datetime.now()
            print("Takeoff detected")
            def thread_function(name):
                logging.info("Thread %s: starting", name)
                print("Waiting for camera switching time...")
                time.sleep(takeoffTime + datetime.timedelta(milliseconds=switchCamerasTime))

                print("Switching cameras")
                
                videoCaptureThread.raise_exception() # Stop existing video capture
                GPIO.setmode(GPIO.BOARD)
                GPIO.setup(26, GPIO.OUT)
                GPIO.output(26, GPIO.HIGH)
                
                print("Switched cameras")
                print("Starting 2nd camera")
                # Start new video capture
                videoCaptureThread = thread_with_exception.thread_with_exception(name=name, target=thread_function, args=(name,))
                logging.info("Thread %s: finishing", name)
            2ndCamThread = threading.Thread(target=thread_function, args=("2ndCam",))
            2ndCamThread.start()

            

calibrate(calibrationFile)
try:
    while True:
        runOneIter()
finally:
    if counter > 0:
        append_list_as_row(my_log, ["Calibration data",avgX/counter,avgY/counter,avgZ/counter])
