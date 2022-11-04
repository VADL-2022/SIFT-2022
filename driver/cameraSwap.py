import pigpio
import time
import sys

pi = pigpio.pi()

cameraDown = int(sys.argv[1]) if len(sys.argv) > 1 else None

PIN = 26

if cameraDown != 0:
    # First camera
    print("hello")
    pi.set_pull_up_down(PIN, pigpio.PUD_OFF) # Clear pull down resistor on pin 26
    pi.set_mode(PIN, pigpio.OUTPUT) # Set pin 26 to output
    pi.write(PIN, 1) # Set pin 26 to high
else:
    # Swap to second camera
    pi.set_mode(PIN, pigpio.INPUT) # Set pin 26 to input
    pi.set_pull_up_down(PIN, pigpio.PUD_DOWN) # Set pin 26 to pull down resistor

exit()
