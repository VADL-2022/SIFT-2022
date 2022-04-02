import serial
import random
import time
import sys
import socket
import datetime

print("Inside radio.py")
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1) # gpio14
ser.reset_input_buffer()

# https://www.reddit.com/r/learnpython/comments/2f3tjh/rounding_updown_datetime_with_specific_resolution/
def round_minutes(dt, direction, resolution):
    new_minute = (dt.minute // resolution + (1 if direction == 'up' else 0)) * resolution
    return dt + datetime.timedelta(minutes=new_minute - dt.minute)

#round_minutes(datetime.datetime.now(),'down',15)
# minutes == 0 then s1 transmit
#round_minutes(datetime.datetime.now(),'down',5) # Assign with repeating assignments

multiple = 5 # Minutes for each pi to send
timetable={
    multiple*0: 'sift1',
    multiple*1: 'sift2',
    multiple*2: 'fore1',
    multiple*3: 'fore2',
    
    multiple*4: 'sift1',
    multiple*5: 'sift2',
    multiple*6: 'fore1',
    multiple*7: 'fore2',
    
    multiple*8: 'sift1',
    multiple*9: 'sift2',
    multiple*10: 'fore1',
    multiple*11: 'fore2',
}

hostname=socket.gethostname()

# Wait for your time to send
while True:
    print("radio.py: Waiting for timetable")
    now = datetime.datetime.now()
    now_noSeconds = now.replace(second=0, microsecond=0)
    rounded=round_minutes(now_noSeconds,'down',multiple)
    whoIsSending = timetable[rounded.minute]
    
    # Get the time left that we can send in
    secondsIntoThisInterval = (now - now_noSeconds).total_seconds() # Current seconds we are late into our sending time
    secondsLeftInThisInterval = multiple*60.0 - secondsIntoThisInterval
    minutesLeftInThisInterval = secondsLeftInThisInterval/60.0
    
    if whoIsSending == hostname:
        print("radio.py: Timetable says it's my turn to send")
        if minutesLeftInThisInterval < 4.5:
            print("radio.py: Not enough time in timetable, sleeping for", secondsLeftInThisInterval, "second(s)")
            time.sleep(secondsLeftInThisInterval)
        else:
            print("radio.py: Timetable says we have enough time to send. Ready to send.")
            break
    else:
        print("radio.py: Waiting for timetable for", secondsLeftInThisInterval, "second(s)")
        time.sleep(secondsLeftInThisInterval)

def ser_write(x):
    ser.write(x)

ser_write(hostname) # Send identifier to radio

def test():
    d = [x for x in range(5)]#range(300)] # range() excludes value given
    
    i=0
    while i < len(d):
        #Data = random.randint(1,4)
        data = d[i] % 10
        res = str(data).encode('utf-8')
        print("Sending test value on radio:",res)
        ser_write(res)
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
        ser_write(res)
    elif sendBytesOfThisFilename is not None:
        res = open(sendBytesOfThisFilename, 'rb').read()
        print("Sending bytes on radio:",res)
        ser_write(res)
    elif useStdin:
        line = input()
        while line:
            res = str(line+'\n').encode('utf-8')
            print("Sending line on radio:",res)
            ser_write(res)
            
            line = input()
    else:
        test()
