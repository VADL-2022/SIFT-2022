import pigpio
import time

# Swap to second camera
pi = pigpio.pi()
pi.set_mode(26, pigpio.INPUT) # Set pin 26 to input
pi.set_pull_up_down(26, pigpio.PUD_DOWN) # Set pin 26 to pull down resistor

time.sleep(6)

pi.set_pull_up_down(26, pigpio.PUD_OFF) # Clear pull down resistor on pin 26
pi.set_mode(26, pigpio.OUTPUT) # Set pin 26 to output
pi.write(26, 1) # Set pin 26 to high

exit()
