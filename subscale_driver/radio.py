import serial
import random
if __name__ == '__main__':
    ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1) # gpio14
    ser.reset_input_buffer()
    
    while True:
        Data = random.randint(1,4)
        ser.write(str(data).encode('utf-8'))
