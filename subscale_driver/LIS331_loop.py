# -*- coding: utf-8 -*-
"""
Created on Thu Jan 27 17:43:47 2022

@author: kdmen

YOU MUST DOWNLOAD SMSBUS ON THE RPI BEFORE YOU CAN RUN THIS:
    
DATA SHEET FOR THIS IMU
https://www.sparkfun.com/datasheets/Sensors/Accelerometer/LIS331HH.pdf

"""

busNum1 = 20
busNum2 = 21

try:
    import smbus
except:
    import smbus2 as smbus
import time
from csv import writer
import time
import logging
import threading
import thread_with_exception
import sys
import numpy as np
from datetime import datetime
from datetime import timedelta
# import RPi.GPIO as GPIO
# GPIO.setmode(GPIO.BOARD)
# GPIO.setup(26, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
# GPIO.setup(26, GPIO.OUT)
# GPIO.output(26, GPIO.LOW) # Switch to first camera
import pigpio
pi = pigpio.pi()
pi.set_mode(26, pigpio.INPUT) # Set pin 26 to input
pi.set_pull_up_down(26, pigpio.PUD_DOWN) # Set pin 26 to pull down resistor
logging.basicConfig(level=logging.INFO)

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
calibrationFile = (sys.argv[4]) if len(sys.argv) > 4 else None
if calibrationFile is None:
    print("Must provide calibrationFile")
    exit(1)
stoppingTime = float(sys.argv[5]) if len(sys.argv) > 5 else None
if not logOnly and stoppingTime is None:
    print("Must provide stoppingTime in milliseconds")
    exit(1)
stopper = None
useLSM_IMU = (sys.argv[6] == '1') if len(sys.argv) > 6 else False

shouldStop=None
import videoCapture
shouldStopMain = videoCapture.AtomicInt(0)
if not logOnly:
    shouldStop=videoCapture.AtomicInt(0)
    def thread_function(name):
        global shouldStop
        logging.info("Thread %s: starting", name)
        videoCapture.run(shouldStop)
        logging.info("Thread %s: finishing", name)
    name="videoCapture"
    videoCaptureThread = thread_with_exception.thread_with_exception(name=name, target=thread_function, args=(name,))
    videoCaptureThread.start()
else:
    videoCaptureThread = None
    name=None

def append_list_as_row(write_obj, list_of_elem):
        # Create a writer object from csv module
        csv_writer = writer(write_obj)
        # Add contents of list as last row in the csv file
        csv_writer.writerow(list_of_elem)

timestr = time.strftime("%Y%m%d-%H%M%S")
my_log = "dataOutput/LOG_" + timestr + ".LIS331.csv"
file_name=my_log

# Create a new (empty) csv file
with open(my_log, 'w', newline='') as file:
    new_file = writer(file)

try:
    if useLSM_IMU:
        bus = smbus.SMBus(busNum1) # 1)
        address = 0x6B

        # SET THE DATA RATE
        ###############################################################################
        # Required Binary: 0101 (Set ODR to 208 Hz), 01 (+/- 16g), 11 (50 Hz Anti Aliasing) --> Equivalent Hex: 01010111 -> 0x57
        bus.write_byte_data(address, 0x10, 0x57)
        # Required Binary: 0101 (Set ODR to 208 Hz), 00 (245 dps), 10 (Set SA0 high) --> Equivalent Hex: 01010010 -> 0x52
        bus.write_byte_data(address, 0x11, 0x52)
    else:
        # Get I2C bus
        bus = smbus.SMBus(busNum1) # 1)
except:
    import traceback
    print("Caught exception from IMU at 1:")
    traceback.print_exc()
    print("Using backup timing parameters")
    bus = None # Means IMU is dead
try:
    # Requires: dtoverlay=i2c-gpio,bus=2,i2c_gpio_sda=22,i2c_gpio_scl=23  in /boot/config.txt (add line) ( https://medium.com/cemac/creating-multiple-i2c-ports-on-a-raspberry-pi-e31ce72a3eb2 )
    # Or run this: `dtoverlay i2c-gpio bus=2 i2c_gpio_sda=22 i2c_gpio_scl=23`
    L3G_bus = smbus.SMBus(busNum2) # 2)
except: # Don't let a gyroscope bring down the whole video capture
    import traceback
    print("Caught exception from L3G at 1:")
    traceback.print_exc()
    
    L3G_bus = None

try:
    if not useLSM_IMU:
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
except:
    import traceback
    print("Caught exception from IMU at 2:")
    traceback.print_exc()
    print("Using backup timing parameters")
    bus = None # Means IMU is dead

# Gyroscope data recording
try:
    if L3G_bus is not None:
        # L3G
        L3G_address=0x6B
        # Required Binary: 0001 (Set ODR to 100 Hz), 0111 (Enable everything) --> Equivalent Hex: 00010111 -> 0x17
        L3G_bus.write_byte_data(L3G_address, 0x20, 0x17)
except: # Don't let a gyroscope bring down the whole video capture
    import traceback
    print("Caught exception from L3G at 2:")
    traceback.print_exc()


time.sleep(0.5)

avgX=0
avgY=0
avgZ=0
counter=0

offsetX=0
offsetY=0
offsetZ=0

def startSwitcher(switchCamerasTime, magnitude, xAccl, yAccl, zAccl, my_accels, shouldStop):
    if my_accels is None:
        print("Switcher using imu failed")
    else:
        print("Takeoff detected with magnitude", magnitude, " m/s^2 and filtered accels", my_accels[1:], "at time", my_accels[0], "seconds (originals:",[xAccl,yAccl,zAccl],")")
    def thread_function2(name):
        global videoCaptureThread
        global shouldStop
        logging.info("Thread %s: starting", name)
        print("Waiting for camera switching time...")
        time.sleep(switchCamerasTime/1000.0)

        print("Switching cameras")

        shouldStop.incrementAndThenGet() #videoCaptureThread.raise_exception() # Stop existing video capture
        videoCaptureThread.join()
        shouldStop.set(0)
        print("GPIO changing...")
        #GPIO.output(26, GPIO.HIGH)
        pi.set_pull_up_down(26, pigpio.PUD_OFF) # Clear pull down resistor on pin 26
        pi.set_mode(26, pigpio.OUTPUT) # Set pin 26 to output
        pi.write(26, 1) # Set pin 26 to high
        print("GPIO sleep...")
        time.sleep(100.0 / 1000.0)

        print("Switched cameras")
        print("Starting 2nd camera")
        # Start new video capture
        videoCaptureThread = thread_with_exception.thread_with_exception(name=name, target=thread_function, args=(name,))
        videoCaptureThread.start()
        logging.info("Thread %s: finishing", name)
    _2ndCamThread = thread_with_exception.thread_with_exception(name=name, target=thread_function2, args=("2ndCam",))
    _2ndCamThread.start()

    def thread_function_stopper(name):
        global shouldStop
        logging.info("Thread %s: starting", name)
        print("Waiting for stopping time...")
        time.sleep(stoppingTime/1000.0)
        print("Stopping")
        shouldStop.incrementAndThenGet()
        logging.info("Thread %s: finishing", name)
        shouldStopMain.incrementAndThenGet()
    stopper = thread_with_exception.thread_with_exception(name="Stopper", target=thread_function_stopper, args=("Stopper",))
    stopper.start()

def calibrate(file):
    global offsetX
    global offsetY
    global offsetZ
    with open(file, 'r') as f:
        for line in f:
            pass
        last_line = line
        offsetX,offsetY,offsetZ=list(map(float, last_line.split(",")[1:]))

start = time.time_ns() # Time since epoch
def runOneIter(write_obj):
    global takeoffTime
    global avgX
    global avgY
    global avgZ
    global counter
    global shouldStop
    
    '''Do I ever need to sleep here? Or will the while take care of it?'''

    try:
        if useLSM_IMU:
            wxL = bus.read_byte_data(address, 0x22)
            wxH = bus.read_byte_data(address, 0x23)
            wx = wxH * 256 + wxL
            if wx > 32767 :
                wx -= 65536

            wyL = bus.read_byte_data(address, 0x24)
            wyH = bus.read_byte_data(address, 0x25)
            wy = wyH * 256 + wyL
            if wy > 32767 :
                wy -= 65536

            wzL = bus.read_byte_data(address, 0x26)
            wzH = bus.read_byte_data(address, 0x27)
            wz = wzH * 256 + wzL
            if wz > 32767 :
                wz -= 65536

            axL = bus.read_byte_data(address, 0x28)
            axH = bus.read_byte_data(address, 0x29)
            ax = axH * 256 + axL
            if ax > 32767 :
                ax -= 65536

            ayL = bus.read_byte_data(address, 0x2A)
            ayH = bus.read_byte_data(address, 0x2B)
            ay = ayH * 256 + ayL
            if ay > 32767 :
                ay -= 65536

            azL = bus.read_byte_data(address, 0x2C)
            azH = bus.read_byte_data(address, 0x2D)
            az = azH * 256 + azL
            if az > 32767 :
                az -= 65536

            time1 = bus.read_byte_data(address, 0x40)
            time2 = bus.read_byte_data(address, 0x41)
            time3 = bus.read_byte_data(address, 0x42)

            current = time.time_ns() # Time since epoch
            currentTime = float(current - start) / 1.0e9 # Then we subtract the start time in nanoseconds and convert to seconds by dividing by 1e9.

            time_ = time3 * 65536 + time2 * 256 + time1

            #my_vals = [ax, ay, az, wx, wy, wz, time] # w = angular rate
            my_vals = [wx, wy, wz, time_] # w = angular rate
            xAccl = ax
            yAccl = ay
            zAccl = az
        else:
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

            current = time.time_ns() # Time since epoch
            currentTime = float(current - start) / 1.0e9 # Then we subtract the start time in nanoseconds and convert to seconds by dividing by 1e9.

            # Convert the data
            zAccl = data1 * 256 + data0
            if zAccl > 32767 :
                zAccl -= 65536

            my_vals = None
    except:
        import traceback
        print("Caught exception from IMU at 3:")
        traceback.print_exc()
        #shouldSleep = takeoffTime + timedelta(milliseconds=switchCamerasTime) < datetime.now()
        destSleep = (takeoffTime + timedelta(milliseconds=switchCamerasTime) - datetime.now()).total_seconds() if takeoffTime is not None else (switchCamerasTime / 1000.0)
        if destSleep > 0:
            print("Using backup timing parameters: sleeping until switchCamerasTime:", destSleep,"seconds")
            time.sleep(destSleep)
            startSwitcher(0, None, None, None, None, None, shouldStop)
            return False
        
    rx=None
    ry=None
    rz=None
    try:
        if L3G_bus is not None:
            # L3GD20H Gyroscope
            ###############################################################################
            rxL = L3G_bus.read_byte_data(L3G_address, 0x28)
            rxH = L3G_bus.read_byte_data(L3G_address, 0x29)
            rx = rxH * 256 + rxL
            if rx > 32767:
                rx -= 65536

            ryL = L3G_bus.read_byte_data(L3G_address, 0x2A)
            ryH = L3G_bus.read_byte_data(L3G_address, 0x2B)
            ry = ryH * 256 + ryL
            if ry > 32767:
                ry -= 65536

            rzL = L3G_bus.read_byte_data(L3G_address, 0x2C)
            rzH = L3G_bus.read_byte_data(L3G_address, 0x2D)
            rz = rzH * 256 + rzL
            if rz > 32767:
                rz -= 65536
    except: # Don't let a gyroscope bring down the whole video capture
        import traceback
        print("Caught exception from L3G at 3:")
        traceback.print_exc()

    # LOG TO CSV
    ###############################################################################
    # Output data to screen
    #print("Acceleration in X-Axis : %d" %xAccl)
    #print("Acceleration in Y-Axis : %d" %yAccl)
    #print("Acceleration in Z-Axis : %d" %zAccl)

    if takeoffTime is None: # Grab running average of near stationary, till takeoff
        avgX += xAccl
        avgY += yAccl
        avgZ += zAccl
        counter += 1
    offsetX = avgX/counter
    offsetY = avgY/counter
    offsetZ = avgZ/counter
    my_accels = [currentTime, (xAccl-offsetX)/1000.0*9.81, (yAccl-offsetY)/1000.0*9.81, (zAccl-offsetZ)/1000.0*9.81]
    resList = [xAccl,yAccl,zAccl,rx,ry,rz]
    if my_vals is not None:
        resList.extend(my_vals)
    append_list_as_row(write_obj, resList)

    # Wait for takeoff
    if not logOnly:
        magnitude = np.linalg.norm(np.array(my_accels[1:]))
        global videoCaptureThread
        global stopper
        if magnitude > takeoffGs*9.81 and takeoffTime is None:
            takeoffTime = datetime.now()
            startSwitcher(switchCamerasTime, magnitude, xAccl, yAccl, zAccl, my_accels, shouldStop)

    return True

            

calibrate(calibrationFile)
try:
    # Open file in append mode
    with open(file_name, 'a+', newline='') as write_obj:
        while shouldStopMain.get() == 0:
            if runOneIter(write_obj) == False:
                # IMU failed
                # Wait forever..
                stopper.join()
except KeyboardInterrupt:
    print("Stopping threads due to keyboard interrupt")
    if shouldStop is not None:
        shouldStop.incrementAndThenGet()
    if videoCaptureThread is not None:
        videoCaptureThread.join()
    if stopper is not None:
        stopper.join()
finally:
    if counter > 0:
        with open(file_name, 'a+', newline='') as write_obj:
            append_list_as_row(write_obj, ["Calibration data",avgX/counter,avgY/counter,avgZ/counter])
