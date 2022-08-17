import os
from gps import *
from time import *
import time
import threading

gpsd = None

class GpsPoller(threading.Thread):
	def __init__(self):
		threading.Thread.__init__(self)
		global gpsd
		gpsd = gps(mode=WATCH_ENABLE)
		self.current_value = None
		self.running = True
	def run(self):
		global gpsd
		while gpsd.running:
			gpsd.next()

if __name__ == '__main__':
	gpsp = GpsPoller()
	try:
		gpsp.start()
		while True:
			print ('lat' , gpsd.fix.latitude)
			print ('long' , gpsd.fix.longitude)
			print ('time' , gpsd.utc, '+' , gpsd.fix.time)
			print ('satellites' , gpsd.satellites)
			time.sleep(1)
	except (KeyboardInterrupt, SystemExit):
		gpsp.running = False
		gpsp.join()
	print ("Done")


