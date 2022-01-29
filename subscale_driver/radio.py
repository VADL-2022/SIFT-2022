import serial
import random
import time
import sys

ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1) # gpio14
ser.reset_input_buffer()

def test():
    d = [x for x in range(300)] # range() excludes value given
    
    i=0
    while i < len(d):
        #Data = random.randint(1,4)
        data = d[i] % 10
        res = str(data).encode('utf-8')
        print(res)
        ser.write(res)
        time.sleep(0.01)

        i+=1

useStdin = (sys.argv[1] == '1') if len(sys.argv) > 1 else False

if __name__ == '__main__':
    if useStdin:
        line = input()
        while line:
            res = str(line).encode('utf-8')
            print("Sending line:",res)
            ser.write(res)
            
            line = input()
    else:
        test()
