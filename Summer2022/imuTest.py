#from smbus2 import smbus
import smbus
import time 

# CONVERSION FACTOR TO M/S^2:
ac_conv_factor = 0.000482283
# CONVERSION FACTOR TO DEG:
g_conv_factor = 0.00072
# ^^Both were empirically determined

def init():
    global bus
    global address
    bus = smbus.SMBus(1)
    time.sleep(1)
    address = 0x6B
    
    # Required Binary: 0101 (Set ODR to 208 Hz), 01 (+/- 16g), 11 (50 Hz Anti Aliasing) --> Equivalent Hex: 01010111 -> 0x57
    bus.write_byte_data(address, 0x10, 0x57)
    # Required Binary: 0101 (Set ODR to 208 Hz), 00 (245 dps), 10 (Set SA0 high) --> Equivalent Hex: 01010010 -> 0x52
    bus.write_byte_data(address, 0x11, 0x52)


def get_accels1():
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
    
    xAccl = ax * ac_conv_factor
    print(xAccl)
    yAccl = ay * ac_conv_factor
    print(yAccl)
    zAccl = az * ac_conv_factor
    print(zAccl)
    tAccl = pow((pow(xAccl, 2) + pow(yAccl, 2) + pow(zAccl, 2)), 1/2)
    print(tAccl)
    
    tAccl = pow((pow(xAccl*9.81, 2) + pow(yAccl*9.81, 2) + pow(zAccl*9.81, 2)), 1/2)
    #print(xAccl*9.81)
    #print(yAccl*9.81)
    #print(zAccl*9.81)
    print(tAccl)


def get_accels2():
    #if useLSM_IMU is False
    
    # LIS data recording ##################################################
    # X AXIS
    ###############################################################################
    # H3LIS331DL address, 0x18(24)
    # Read data back from 0x28(40), 2 bytes
    # X-Axis LSB, X-Axis MSB
    data0 = bus.read_byte_data(address, 0x28)
    data1 = bus.read_byte_data(address, 0x29)

    # Convert the data
    xAccl = data1 * 256 + data0
    if xAccl > 32767 :
        xAccl -= 65536


    # Y AXIS
    ###############################################################################
    # H3LIS331DL address, 0x18(24)
    # Read data back from 0x2A(42), 2 bytes
    # Y-Axis LSB, Y-Axis MSB
    data0 = bus.read_byte_data(address, 0x2A)
    data1 = bus.read_byte_data(address, 0x2B)

    # Convert the data
    yAccl = data1 * 256 + data0
    if yAccl > 32767 :
        yAccl -= 65536


    # Z AXIS
    ###############################################################################
    # H3LIS331DL address, 0x18(24)
    # Read data back from 0x2C(44), 2 bytes
    # Z-Axis LSB, Z-Axis MSB
    data0 = bus.read_byte_data(address, 0x2C)
    data1 = bus.read_byte_data(address, 0x2D)

    #current = time.time_ns() # Time since epoch
    #currentTime = float(current - start) / 1.0e9 # Then we subtract the start time in nanoseconds and convert to seconds by dividing by 1e9.

    # Convert the data
    zAccl = data1 * 256 + data0
    if zAccl > 32767 :
        zAccl -= 65536

    my_vals = None
    
    tAccl = pow((pow(xAccl, 2) + pow(yAccl, 2) + pow(zAccl, 2)), 1/2)
    
    #print(xAccl)
    #print(yAccl)
    #print(zAccl)
    #print(tAccl)
    

    
    


init()
get_accels1()
#get_accels2()

