import pigpio
import time
import sys

pi = pigpio.pi()

cameraDown = (sys.argv[1] == '1') if len(sys.argv) > 1 else None

if cameraDown is 0:
    # First camera
    pi.set_pull_up_down(26, pigpio.PUD_OFF) # Clear pull down resistor on pin 26
    pi.set_mode(26, pigpio.OUTPUT) # Set pin 26 to output
    pi.write(26, 1) # Set pin 26 to high
else:
    # Swap to second camera
    pi.set_mode(26, pigpio.INPUT) # Set pin 26 to input
    pi.set_pull_up_down(26, pigpio.PUD_DOWN) # Set pin 26 to pull down resistor

exit()
