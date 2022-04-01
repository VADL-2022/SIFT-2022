import serial
import random
import time
import sys

print("Inside radio.py")
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1) # gpio14
ser.reset_input_buffer()

def test():
    d = [x for x in range(5)]#range(300)] # range() excludes value given
    
    i=0
    while i < len(d):
        #Data = random.randint(1,4)
        data = d[i] % 10
        res = str(data).encode('utf-8')
        print("Sending test value on radio:",res)
        ser.write(res)
        time.sleep(0.1)

        i+=1

useStdin = (sys.argv[1] == '1') if len(sys.argv) > 1 else False
sendThisString = (sys.argv[2]) if len(sys.argv) > 2 and sys.argv[2] != '' else None
sendBytesOfThisFilename = (sys.argv[3]) if len(sys.argv) > 3 else None

#if __name__ == '__main__':
if True:
    if sendThisString is not None:
        res = str(sendThisString).encode('utf-8')
        print("Sending string on radio:",res)
        ser.write(res)
    elif sendBytesOfThisFilename is not None:
        res = open(sendBytesOfThisFilename, 'rb').read()
        print("Sending bytes on radio:",res)
        ser.write(res)
    elif useStdin:
        line = input()
        while line:
            res = str(line+'\n').encode('utf-8')
            print("Sending line on radio:",res)
            ser.write(res)
            
            line = input()
    else:
        test()
