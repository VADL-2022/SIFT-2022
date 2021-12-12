import serial
import random
d = [x for x in range(300)] # range() excludes value given
if __name__ == '__main__':
    ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1) # gpio14
    ser.reset_input_buffer()
    
    while i < len(d):
        #Data = random.randint(1,4)
        data = d[i]
        ser.write(str(data).encode('utf-8'))
        
        i+=1
