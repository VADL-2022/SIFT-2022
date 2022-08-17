#from smbus2 import smbus
import smbus
import time 
import videoCapture
import logging
import time

def start_recording():
    global name
    global shouldStop
    
    name = 'fore1';
    shouldStop = videoCapture.AtomicInt(0)

    logging.info("Thread %s: starting", name)
    videoCapture.run(shouldStop, False, None, 30, 1280, 720, None, None)
    logging.info("Thread %s: finishing", name)

def stop_recording():
    shouldStop.set(1)



def timed_recording(start_time, recording_time):
    if time.time() - start_time > recording_time:
        shouldStop = videoCapture.AtomicInt(1)
        return True
    return False



def init_imu():
    global bus
    global address
    bus = smbus.SMBus(1)
    time.sleep(1)
    address = 0x6B
    
    # Required Binary: 0101 (Set ODR to 208 Hz), 01 (+/- 16g), 11 (50 Hz Anti Aliasing) --> Equivalent Hex: 01010111 -> 0x57
    bus.write_byte_data(address, 0x10, 0x57)
    # Required Binary: 0101 (Set ODR to 208 Hz), 00 (245 dps), 10 (Set SA0 high) --> Equivalent Hex: 01010010 -> 0x52
    bus.write_byte_data(address, 0x11, 0x52)



def get_accels():
    ac_conv_factor = 0.000482283
    
    #if useLSM_IMU is True
    axL = bus.read_byte_data(address, 0x28)
    axH = bus.read_byte_data(address, 0x29)
    ax = axH * 256 + axL
    if ax > 32767 :
        ax -= 65536

    ayL = bus.read_byte_data(address, 0x2A)
    ayH = bus.read_byte_data(address, 0x2B)
    ay = ayH * 256 + ayL
    if ay > 32767 :
        ay -= 65536

    azL = bus.read_byte_data(address, 0x2C)
    azH = bus.read_byte_data(address, 0x2D)
    az = azH * 256 + azL
    if az > 32767 :
        az -= 65536
    
    xAccl = ax * ac_conv_factor * 9.81
    yAccl = ay * ac_conv_factor * 9.81
    zAccl = az * ac_conv_factor * 9.81
    tAccl = pow((pow(xAccl, 2) + pow(yAccl, 2) + pow(zAccl, 2)), 1/2)
    #print(xAccl)
    #print(yAccl)
    #print(zAccl)
    #print(tAccl)
    
    return [xAccl, yAccl, zAccl, tAccl]
    
    
    
def init_avg_accels(number_samples, index):
    out = []
    for _ in range(number_samples):
        current_accels = get_accels()
        out.append(current_accels[index])
    return out
    


def main():
    start_time = time.time()
    recording_time = 10
    
    launched = False
    launch_g = 1
    accel_roof = launch_g * 9.81
    launch_samples = 15
    
    landing_samples = 60
    
    tAccel_index = 3

    init_imu()
    accels = init_avg_accels(launch_samples, tAccel_index)

    while not launched:
        accels.pop(0)
        accels.append(get_accels()[tAccel_index])

        if sum(accels)/len(accels) > accel_roof:
            launched = True

    start_recording()
    
    con = True
    while con:
        etime = time.time() - start_time
        print(etime)
        if etime > 3:
            stop_recording()
            print("stop recording please")
            con = False

if __name__ == "__main__":
    main()
