import videoCapture
import logging


name = 'fore1';
shouldStop = videoCapture.AtomicInt(0)

logging.info("Thread %s: starting", name)
videoCapture.run(shouldStop, False, None, 15, 1280, 720, None, None)
logging.info("Thread %s: finishing", name)