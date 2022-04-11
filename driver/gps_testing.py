#!/usr/bin/env python
import time
import serial

ser = serial.Serial(
        port='/dev/ttyAMA0',
        baudrate = 9600,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=2
)


#timestr = time.strftime("%Y%m%d-%H%M%S")
#my_log = "dataOutput/LOG_" + timestr + ".gps.txt"
#file_name=my_log

# Create a new (empty) csv file
#with open(my_log, 'w', newline='') as file:
#    for i in range(0,2):
#        x=ser.readline()
#        print(str(x))
#        file.write (str(x))
#        file.write("\n")

while 1:
	x=ser.readline()
	print(x)
