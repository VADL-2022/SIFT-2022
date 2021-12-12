import serial
import random
d = [x for x in range(300)] # range() excludes value given
if __name__ == '__main__':
    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1) # gpio14
    ser.reset_input_buffer()

    i=0
    while i < len(d):
        #Data = random.randint(1,4)
        data = d[i]
        res = str(data).encode('utf-8')
        print(res)
        ser.write(res)
        
        i+=1
