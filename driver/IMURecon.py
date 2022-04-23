import numpy as np
from pandas import read_csv
from math import sin, cos, pi, atan2, asin, sqrt, ceil


def calc_displacement2(imu_data_file_and_path, launch_rail_box, weather_station_stats_xy, GPS_coords=0, my_thresh=45, my_post_drogue_delay=0.5, my_signal_length=3, ld_launch_angle=5*pi/180, ld_ssm=2.44, ld_dry_base=15.735, ld_m_motor=1.728187, ld_t_burn=2.05, ld_T_avg=1771):
    '''
    This is the main function, the rest are all nested functions called by this one.
    
    REQUIRED PARAMETERS
    imu_data_file_and_path: string containing the file name and location of the IMU CSV file.  E.g. "../../Data/VN_LOG_1234567.csv"
    launch_rail_box: grid box number that the launch rail is in.  Probably will only know this on launch day, so pass in argument
    weather_station_stats_xy: the wind speed of the x and y directions as given by the weather report
    
    ########################################################################################
    #######################    BIG RED FLAG DEPENDENCIES/NOTES    ##########################
    ########################################################################################
    HUGE DEPENDENCY ON THIS FUNCTION RUNNING 120 SECONDS AFTER TAKEOFF
    IF IT IS CALLED BEFORE THAT, IT WILL THROW AN INDEX ERROR BECAUSE IT FINDS LANDING BY FINDING 
     THE ALTITUDE AT TIME 120 AND WORKING BACKWARDS
    
    IF YOU WANT THE GPS TO RUN, YOU MUST SET GPS_COORDS TO THE GPS VALUES, VIA [LONGITUDE, LATITUDE]
    IF YOU LEAVE GPS AS ITS DEFAULT PARAM THEN IT WILL NOT RUN
    ########################################################################################
    
    YOU NEED TO UPDATE THESE ON LAUNCH DAY
    ld_launch_angle: launch day launch angle (e.g. angle of the launch rail)
    ld_ssm: launch day ssm
    ld_dry_base: dry mass of the launch day vehicle NOT INCLUDING THE MOTOR
    ld_m_motor: motor mass, change for fullscale vs subscale
    ld_t_burn: motor burn time, change for fullscale vs subscale
    ld_T_avg: motor thrust, change for fullscale vs subscale
    
    PROBABLY DONT NEED TO CHANGE THESE
    my_thresh: acceleration threshold to check for takeoff.  Currently 45 m/s2
    my_post_drogue_delay: time after apogee to wait for 0 acceleration transience to pass
    ^ Use 0.5 for fullscale, 0.85 for subscales
    my_signal_length: length of analysis window, 3 seconds
    
    RETURNS
    all_xs: a LIST of grid numbers that we think we landed in.  First is the "average" NOT BEST, the rest are the uncertainty
    Now also returns:
        all_GPS_boxes: a lits of all the grid numbers we could've landed in, using the GPS data directly (e.g. launch rail GPS)
        the original wind speeds
        the updated wind speeds
        all displacements
        landing/signal times
    
    NOTES
        We output a lot of text for quick sanity checks.  Could toggle this with a verbose mode (don't have yet).  Ideally this is 
        simply saved to a text file for us to check post launch or in the lab to evaluate the flight.
    '''

    ## DETECT PARAMETERS
    take_off_threshold_g = my_thresh;
    landing_threshold_g = my_thresh;

    ## ALTITUDE PARAMETER
    B = 6.5e-3  # temperature lapse rate in troposphere in K/m
    R = 287  # ideal gas constant in J/(kg.K)
    g = 9.80665  # gravity at sea level in m/s2
    T_0 = 288.15  # standard air temperature in K
    P_0 = 101.325  # standard air pressure in kPa
    # Parameters
    dt = 0.001
    pi = 3.1415
    ft = 3.2884  # ft/m
    
    my_arbitrary_wind_threshold = 0.8

    ## IMU PROCESS
    # Read in the dataframe
    fields = ['Timestamp', 'Pres',
    'Roll', 'Pitch', 'Yaw',
    'LinearAccelNed X', 'LinearAccelNed Y', 'LinearAccelNed Z']
    df = read_csv(imu_data_file_and_path, skipinitialspace=True, usecols=fields)

    ## EXTRACT TIME, ACCEL, AND ALTITUDE
    imu_t = np.array(df['Timestamp'].values)
    imu_t = imu_t - imu_t[0]
    imu_ax = np.array(df['LinearAccelNed X'])
    imu_ay= np.array(df['LinearAccelNed Y'])
    imu_az = np.array(df['LinearAccelNed Z']*-1)
    imu_a = imu_ax**2 + imu_ay**2 + imu_az**2
    imu_a = [sqrt(val) for val in imu_a]
    imu_N = len(imu_t);
    imu_pres = np.array(df['Pres'])

    # Vectorize this...
    imu_temp = T_0*(imu_pres/P_0)**(R*B/g);
    imu_alt = (T_0 - imu_temp)/B;

    ## FIND TAKEOFF AND UPDATE THE ARRAYS
    # Subtract 3 to slightly step back to make sure we capture the full spike
    take_off_i = np.argmax(np.array(imu_a)>take_off_threshold_g) - 3

    imu_t = imu_t[take_off_i:imu_N];
    imu_t = imu_t - imu_t[0];

    imu_ax = imu_ax[take_off_i:imu_N]; 
    imu_ay = imu_ay[take_off_i:imu_N];
    imu_az = imu_az[take_off_i:imu_N];
    imu_a = imu_a[take_off_i:imu_N];

    imu_alt = imu_alt[take_off_i:imu_N];
    imu_alt = imu_alt - imu_alt[1];
    imu_alt[0] = 0
    imu_alt = [val if val>0 else 0 for val in imu_alt]

    take_off_i = 0

    # NEW ALTITUDE BASED LANDING DETECTION
    # This really ought to propgate forward from like 50 seconds
    # This introduces a big dependency on reading the file 120 seconds after takeoff...
    lateLandingIndex = np.argmin((abs(imu_t - 120)))

    lateAltitude = imu_alt[lateLandingIndex]
    i = lateLandingIndex
    
    while imu_alt[i] < (lateAltitude + 10):  # 10m of uncertainty
        i -= 10
            
    landing_i = i + 100

    imu_t = imu_t[0:landing_i];
    imu_ax = imu_ax[0:landing_i]; 
    imu_ay = imu_ay[0:landing_i];
    imu_az = imu_az[0:landing_i];
    imu_a = imu_a[0:landing_i];
    imu_alt = imu_alt[0:landing_i];
    imu_N = len(imu_t)

    ## DISPLAY RESULTS
    print(f"Take-Off Time (s) = {imu_t[take_off_i]}");
    print(f"Landing (s) = {imu_t[landing_i-1]}");

    imu_vz = np.zeros(imu_N)
    imu_z = np.zeros(imu_N)

    imu_vy = np.zeros(imu_N)
    imu_y = np.zeros(imu_N)

    imu_vx = np.zeros(imu_N)
    imu_x = np.zeros(imu_N)

    ################## Find velocity and position  ##################
    for i in range(len(imu_t)-1):
        imu_vz[i+1] = imu_vz[i] + imu_az[i]*(imu_t[i+1] - imu_t[i])
        imu_z[i+1] = imu_z[i] + imu_vz[i]*(imu_t[i+1] - imu_t[i])

        imu_vx[i+1] = imu_vx[i] + imu_ax[i]*(imu_t[i+1] - imu_t[i])
        imu_x[i+1] = imu_x[i] + imu_vx[i]*(imu_t[i+1] - imu_t[i])

        imu_vy[i+1] = imu_vy[i] + imu_ay[i]*(imu_t[i+1] - imu_t[i])
        imu_y[i+1] = imu_y[i] + imu_vy[i]*(imu_t[i+1] - imu_t[i])

    # Find the max altitude
    z_0 = max(imu_alt)
    apogee_idx = list(imu_alt).index(z_0)

    # Alternatively, we could also add 1 second to the apogee_idx and find the corresponding index
    temp = imu_t - imu_t[apogee_idx]  - my_post_drogue_delay
    masked_temp = np.array([val if abs(val)>10**-5 else 0 for val in temp])
    my_min = min(abs(masked_temp))
    if my_min < 10**-5:
        my_min = 0
    try:
        imu_start_time = list(masked_temp).index(my_min)
    except ValueError:
        imu_start_time = list(masked_temp).index(-my_min)
    print(f"Start time of signal: {imu_t[imu_start_time]}")

    temp = imu_t - imu_t[apogee_idx] - my_post_drogue_delay - my_signal_length
    masked_temp = np.array([val if abs(val)>10**-5 else 0 for val in temp])
    my_min = min(abs(masked_temp))
    if my_min < 10**-5:
        my_min = 0
    try:
        imu_end_time = list(masked_temp).index(my_min)
    except ValueError:
        imu_end_time = list(masked_temp).index(-my_min)
    print(f"End time of signal: {imu_t[imu_end_time]}")

    w0x = imu_vx[imu_end_time]-imu_vx[imu_start_time]
    w0y = imu_vy[imu_end_time]-imu_vy[imu_start_time]
    print(f"IMU WIND SPEEDS, X->{w0x} m/s and Y->{w0y} m/s")
    
    wx_ws, wy_ws = weather_station_stats_xy[0]/2.237, weather_station_stats_xy[1]/2.237
    wx_ws_apogee, wy_ws_apogee = wx_ws*((z_0/2)**(1/7)), wy_ws*((z_0/2)**(1/7))
    
    # If we flip directions then just set it to zero, there wasn't actually any wind most likely
    # IF THE SIGNS ARE OPPOSITE, WE TRUST THE WEATHER REPORT OVER THE IMU
    #abs(wx_ws)>my_arbitrary_wind_threshold and 
    if wx_ws*w0x < 0:
        w0x = wx_ws_apogee
    if wy_ws*w0y < 0:
        w0y = wy_ws_apogee
    print(f"UPDATED WIND SPEEDS (weather report), X->{w0x} m/s and Y->{w0y} m/s")
    print()
    
    m1_final_x_displacements, m1_final_y_displacements = [0]*3, [0]*3
    m2_final_x_displacements, m2_final_y_displacements = [0]*3, [0]*3
    
    if (abs(w0x) < my_arbitrary_wind_threshold and abs(w0y) > my_arbitrary_wind_threshold):
        w0x = 0
        # LOOP 1
        print("LOOP 1")
        print("X too small, Y is big enough")
        print(f"UPDATED WIND SPEEDS (setting to 0), X->{w0x} m/s and Y->{w0y} m/s")

        drogue_opening_displacement_y = imu_y[imu_end_time] - imu_y[imu_start_time]

        for idx, uncertainty in enumerate([-1, 0, 1]):
            #For end_time to landing
            total_y_displacement = 0

            adj_wy = w0y+uncertainty
            adj_wx = w0x

            # If we flip directions then just set it to zero, there wasn't actually any wind most likely
            # SHOULD THIS BE GREATER OR LESS THAN THE THRESHOLD
            # WHY ARE WE SETTING TO ZERO?
            #abs(adj_wy) > my_arbitrary_wind_threshold and 
            if adj_wy*w0y < 0:
                adj_wy = 0

            for i in range(imu_end_time, landing_i):
                vy = (adj_wy)*((imu_alt[i]/z_0)**(1/7))
                total_y_displacement += vy*(imu_t[i] - imu_t[i-1])

            # Oz Ascent Model
            m1_final_x_displacements[idx] = (imu_x[imu_start_time] - imu_x[0])
            m1_final_y_displacements[idx] = (imu_y[imu_start_time] - imu_y[0]) + drogue_opening_displacement_y + total_y_displacement

            # Oz's Other Ascent Model (Model 2) In Place of Marissa's Model
            landing_i -= 1
            m2y = oz_ascent_model2(abs(adj_wy), imu_alt, imu_t, my_theta=ld_launch_angle, my_ssm=ld_ssm, my_dry_base=ld_dry_base, my_max_sim_time=imu_t[landing_i], my_m_motor=ld_m_motor, my_t_burn=ld_t_burn, my_T_avg=ld_T_avg)[-1]
            print(f"Model2 y displacement: {m2y}")

            # The model and the actual windspeed need to have opposite signs
            if m2y*adj_wy > 0:
                m2y *= -1
                print(f"SIGN FLIPPED: Model2 y displacement: {m2y}")

            m2_final_x_displacements[idx] = 0
            m2_final_y_displacements[idx] = m2y + drogue_opening_displacement_y + total_y_displacement

            print(f"MODEL 1: TOTAL X AND Y DISPLACEMENTS, u={uncertainty}: X->{m1_final_x_displacements[idx]:2f} m, Y->{m1_final_y_displacements[idx]:2f} m")
            print(f"MODEL 2: TOTAL X AND Y DISPLACEMENTS, u={uncertainty}: X->{m2_final_x_displacements[idx]:2f} m, Y->{m2_final_y_displacements[idx]:2f} m")
            print()

    elif (abs(w0y) < 1 and abs(w0x) > 1):
        w0y = 0
        # LOOP 2
        print("LOOP 2")
        print("Y too small, X is big enough")
        print(f"UPDATED WIND SPEEDS (setting to 0), X->{w0x} m/s and Y->{w0y} m/s")
        
        drogue_opening_displacement_x = imu_x[imu_end_time] - imu_x[imu_start_time]

        for idx, uncertainty in enumerate([-1, 0, 1]):
            #For end_time to landing
            total_x_displacement = 0
            adj_wx = w0x+uncertainty
            adj_wy = w0y

            # If we flip directions then just set it to zero, there wasn't actually any wind most likely
            if adj_wx*w0x < 0:
                adj_wx = 0

            for i in range(imu_end_time, landing_i):
                vx = (adj_wx)*((imu_alt[i]/z_0)**(1/7))
                total_x_displacement += vx*(imu_t[i] - imu_t[i-1])

            # Oz Ascent Model
            m1_final_x_displacements[idx] = (imu_x[imu_start_time] - imu_x[0]) + drogue_opening_displacement_x + total_x_displacement
            m1_final_y_displacements[idx] = (imu_y[imu_start_time] - imu_y[0])

            # Oz's Other Ascent Model (Model 2) In Place of Marissa's Model
            landing_i -= 1
            m2x = oz_ascent_model2(abs(adj_wx), imu_alt, imu_t, my_theta=ld_launch_angle, my_ssm=ld_ssm, my_dry_base=ld_dry_base, my_max_sim_time=imu_t[landing_i], my_m_motor=ld_m_motor, my_t_burn=ld_t_burn, my_T_avg=ld_T_avg)[-1]
            m2y = 0
            print(f"Model2 x displacement: {m2x}")
            print(f"Model2 y displacement: {m2y}")

            # The model and the actual windspeed need to have opposite signs
            if m2x*adj_wx > 0:
                m2x *= -1
                print(f"SIGN FLIPPED: Model2 x displacement: {m2x}")

            m2_final_x_displacements[idx] = m2x + drogue_opening_displacement_x + total_x_displacement
            m2_final_y_displacements[idx] = 0

            print(f"MODEL 1: TOTAL X AND Y DISPLACEMENTS, u={uncertainty}: X->{m1_final_x_displacements[idx]:2f} m, Y->{m1_final_y_displacements[idx]:2f} m")
            print(f"MODEL 2: TOTAL X AND Y DISPLACEMENTS, u={uncertainty}: X->{m2_final_x_displacements[idx]:2f} m, Y->{m2_final_y_displacements[idx]:2f} m")
            print()  
        
    elif (abs(w0y) < 1 and abs(w0x) < 1):
        # LOOP 3
        print("LOOP 3")
        print("X and Y both too small")
        
        adj_wx = 0
        adj_wy = 0
        
        print(f"UPDATED WIND SPEEDS (setting to 0), X->{adj_wx} m/s and Y->{adj_wy} m/s")

        for i in range(3):
            # Oz Ascent Model
            m1_final_x_displacements[i] = (imu_x[imu_start_time] - imu_x[0])
            m1_final_y_displacements[i] = (imu_y[imu_start_time] - imu_y[0])

        print(f"MODEL 1: TOTAL X AND Y DISPLACEMENTS: X->{m1_final_x_displacements[0]:2f} m, Y->{m1_final_y_displacements[0]:2f} m")
        print()

    #elif (abs(w0y) > 1 and abs(w0x) > 1):
    else:
        # LOOP 4
        print("LOOP 4")
        print("X and Y are acceptable")
        
        drogue_opening_displacement_x = imu_x[imu_end_time] - imu_x[imu_start_time]
        drogue_opening_displacement_y = imu_y[imu_end_time] - imu_y[imu_start_time]

        # Fix the index from MATLAB to Python
        landing_i -= 1

        for idx, uncertainty in enumerate([-1, 0, 1]):
            #For end_time to landing
            total_x_displacement = 0
            total_y_displacement = 0

            adj_wx = w0x+uncertainty
            adj_wy = w0y+uncertainty

            # If we flip directions then just set it to zero, there wasn't actually any wind most likely
            if adj_wx*w0x < 0:
                print("Adjusted adj_wx sign")
                adj_wx = 0
            if adj_wy*w0y < 0:
                print("Adjusted adj_wy sign")
                adj_wy = 0

            for i in range(imu_end_time, landing_i):
                vx = (adj_wx)*((imu_alt[i]/z_0)**(1/7))
                vy = (adj_wy)*((imu_alt[i]/z_0)**(1/7))
                total_y_displacement += vy*(imu_t[i] - imu_t[i-1])
                total_x_displacement += vx*(imu_t[i] - imu_t[i-1])

            # Oz Ascent Model
            m1_final_x_displacements[idx] = (imu_x[imu_start_time] - imu_x[0]) + drogue_opening_displacement_x + total_x_displacement
            m1_final_y_displacements[idx] = (imu_y[imu_start_time] - imu_y[0]) + drogue_opening_displacement_y + total_y_displacement

            # Oz's Other Ascent Model (Model 2) In Place of Marissa's Model
            m2x = oz_ascent_model2(abs(adj_wx), imu_alt, imu_t, my_theta=ld_launch_angle, my_ssm=ld_ssm, my_dry_base=ld_dry_base, my_max_sim_time=imu_t[landing_i], my_m_motor=ld_m_motor, my_t_burn=ld_t_burn, my_T_avg=ld_T_avg)[-1]
            m2y = oz_ascent_model2(abs(adj_wy), imu_alt, imu_t, my_theta=ld_launch_angle, my_ssm=ld_ssm, my_dry_base=ld_dry_base, my_max_sim_time=imu_t[landing_i], my_m_motor=ld_m_motor, my_t_burn=ld_t_burn, my_T_avg=ld_T_avg)[-1]
            print(f"Model2 x displacement: {m2x}")
            print(f"Model2 y displacement: {m2y}")

            # The model and the actual windspeed need to have opposite signs
            if m2x*adj_wx > 0:
                print("Adjusted m2x sign")
                m2x *= -1
            if m2y*adj_wy > 0:
                print("Adjusted m2y sign")
                m2y *= -1

            m2_final_x_displacements[idx] = m2x + drogue_opening_displacement_x + total_x_displacement
            m2_final_y_displacements[idx] = m2y + drogue_opening_displacement_y + total_y_displacement

            print(f"MODEL 1: TOTAL X AND Y DISPLACEMENTS, u={uncertainty}: X->{m1_final_x_displacements[idx]:2f} m, Y->{m1_final_y_displacements[idx]:2f} m")
            print(f"MODEL 2: TOTAL X AND Y DISPLACEMENTS, u={uncertainty}: X->{m2_final_x_displacements[idx]:2f} m, Y->{m2_final_y_displacements[idx]:2f} m")
            print()

    all_xs = []
    all_xs.extend(m1_final_x_displacements)
    all_xs.extend(m2_final_x_displacements)

    all_ys = []
    all_ys.extend(m1_final_y_displacements)
    all_ys.extend(m2_final_y_displacements)

    minx = min(all_xs)
    maxx = max(all_xs)
    avg_x = (minx+maxx)/2

    miny = min(all_ys)
    maxy = max(all_ys)
    avg_y = (miny+maxy)/2

    print("---------------------------------------------------------------")
    print()

    print(f"Minimum-Maximum x: {minx} - {maxx} (m), u_range={maxx-minx} (m)")
    print(f"Minimum-Maximum y: {miny} - {maxy} (m), u_range={maxy-miny} (m)")

    print(f"Avg X displacement: {avg_x} (m)") 
    print(f"Avg Y displacement: {avg_y} (m)") 
    
    print("---------------------------------------------------------------")
    print("METHOD 1: Original (Center Assumed) Approach:")
        
    new_xbox = update_xboxes(-avg_x, launch_rail_box)
    final_grid_number = update_yboxes(avg_y, new_xbox)
    
    # Somewhat shoddy logic
    if (maxx-minx)/2 > 250/ft:
        all_xs = [final_grid_number, final_grid_number+20, final_grid_number-20]
    else:
        all_xs = [final_grid_number]
    if (maxy-miny)/2 > 250/ft:
        all_xs.append(final_grid_number+1)
        all_xs.append(final_grid_number-1)
    
    print(f"Started in grid number {launch_rail_box}, ended in {final_grid_number} (average)")
    print(f"ALL OG GRID BOXES: {all_xs}")
    
    print()
    
    print("---------------------------------------------------------------")
    if GPS_coords == 0:
        print("DID NOT INPUT GPS COORDS, IGNORING MODEL 2")
        all_GPS_boxes = -2
    else:
        print("METHOD 2: GPS Approach:")
        all_GPS_boxes = GPS_to_grid_box(maxx, minx, avg_x, maxy, miny, avg_y, input_longitude=GPS_coords[0], input_latitude=GPS_coords[1])
        print(f"ALL GPS GRID BOXES: {all_GPS_boxes}")
        
    return all_xs[0], all_GPS_boxes[0], round(adj_wx, 2), round(adj_wy, 2), round((maxx-minx)/2, 2), round((maxy-miny)/2, 2), round(imu_t[landing_i-1], 2)


def GPS_to_grid_box(max_x, min_x, average_x, min_y, max_y, average_y, input_longitude=34.894277, input_latitude=-86.616216):
    input_latitude = abs(input_latitude)
    
    ## DETERMINE LAUNCH GRID
    n = 20;
    index_base = np.array(range(n*n))
    index_matrix = np.reshape(index_base, (n,n))
    
    ## Hard code for OUR Braggs Farm image
    longitude_start = 34.900091;
    latitude_start = 86.624884;
    longitude_end = 34.886289;
    latitude_end = 86.608062;

    longitude_interval = abs(longitude_start-longitude_end)/n;
    latitude_interval = abs(latitude_start-latitude_end)/n;

    longitude_matrix = np.zeros((n,n));
    latitude_matrix = np.zeros((n,n));

    temp_long = longitude_start;

    for i in range(n):
        for j in range(n):
            longitude_matrix[i, j] = temp_long;
            latitude_matrix[i, j] = latitude_start - (j)*latitude_interval;
        temp_long = temp_long - longitude_interval;

    launch_grid = -1;

    for i in range(n):
        for j in range(n):
            if ((longitude_matrix[i, j] >= input_longitude) and (input_longitude >= (longitude_matrix[i, j] - longitude_interval))) and ((latitude_matrix[i, j] >= input_latitude) and (input_latitude >= (latitude_matrix[i, j] - latitude_interval))):
                launch_grid = index_matrix[i, j];

    print("Launch Grid: " + str(launch_grid));

    grid_dimension = 1524;
    long_to_gps = abs(longitude_start-longitude_end)/grid_dimension; 
    lat_to_gps = abs(latitude_start-latitude_end)/grid_dimension; 

    average_x_gps = long_to_gps*average_x + input_longitude; 
    average_y_gps = -lat_to_gps*average_y + input_latitude; 

    max_x_gps = long_to_gps*max_x + input_longitude; 
    max_y_gps = lat_to_gps*max_y + input_latitude; 

    min_x_gps = long_to_gps*min_x + input_longitude; 
    min_y_gps = lat_to_gps*min_y + input_latitude; 

    average_landing_grid = -1;

    # AVERAGE
    for i in range(n):
        for j in range(n):
            if ((longitude_matrix[i, j] >= average_x_gps) and (average_x_gps >= (longitude_matrix[i, j] - longitude_interval))) and ((latitude_matrix[i, j] >= average_y_gps) and (average_y_gps >= (latitude_matrix[i, j] - latitude_interval))):

                average_landing_grid = index_matrix[i, j];
                
    ## CALCULATE THE UNCERTAINTIES
    x_u = abs(max_x - min_x)/2
    y_u = abs(max_y - min_y)/2
    
    north_x_gps = long_to_gps*x_u + average_x_gps; 
    north_y_gps = average_y_gps; 

    west_x_gps = average_x_gps; 
    west_y_gps = average_y_gps + lat_to_gps*y_u;
    
    south_x_gps = -long_to_gps*x_u + average_x_gps;  
    south_y_gps = average_y_gps;  

    east_x_gps = average_x_gps; 
    east_y_gps = average_y_gps - lat_to_gps*y_u;
    
    north_landing_grid = -1;
    west_landing_grid = -1;
    south_landing_grid = -1;
    east_landing_grid = -1;
    
    # NORTH            
    for i in range(n):
        for j in range(n):
            if ((longitude_matrix[i, j] >= north_x_gps) and (north_x_gps >= (longitude_matrix[i, j] - longitude_interval))) and ((latitude_matrix[i, j] >= north_y_gps) and (north_y_gps >= (latitude_matrix[i, j] - latitude_interval))):
                north_landing_grid = index_matrix[i, j];
    # WEST          
    for i in range(n):
        for j in range(n):
            if ((longitude_matrix[i, j] >= west_x_gps) and (west_x_gps >= (longitude_matrix[i, j] - longitude_interval))) and ((latitude_matrix[i, j] >= west_y_gps) and (west_y_gps >= (latitude_matrix[i, j] - latitude_interval))):
                west_landing_grid = index_matrix[i, j];
    # SOUTH          
    for i in range(n):
        for j in range(n):
            if ((longitude_matrix[i, j] >= south_x_gps) and (south_x_gps >= (longitude_matrix[i, j] - longitude_interval))) and ((latitude_matrix[i, j] >= south_y_gps) and (south_y_gps >= (latitude_matrix[i, j] - latitude_interval))):
                south_landing_grid = index_matrix[i, j];
    # EAST          
    for i in range(n):
        for j in range(n):
            if ((longitude_matrix[i, j] >= east_x_gps) and (east_x_gps >= (longitude_matrix[i, j] - longitude_interval))) and ((latitude_matrix[i, j] >= east_y_gps) and (east_y_gps >= (latitude_matrix[i, j] - latitude_interval))):
                east_landing_grid = index_matrix[i, j];

    print("Average Landing Grid: " + str(average_landing_grid));
    print("North Landing Grid: " + str(north_landing_grid));
    print("West Landing Grid: " + str(west_landing_grid));
    print("South Landing Grid: " + str(south_landing_grid));
    print("East Landing Grid: " + str(east_landing_grid));
    
    return [average_landing_grid, north_landing_grid, west_landing_grid, south_landing_grid, east_landing_grid]


def update_xboxes(avg_x, launch_rail_box):
    ft = 3.2884
    avg_x *= ft
    
    if abs(avg_x) > 125:
        if avg_x > 0:
            out_of_xbox = avg_x - 125
            if abs(out_of_xbox) > 0:
                num_xboxes = ceil(out_of_xbox/250)
            else:
                num_xboxes = 0
            new_xbox = launch_rail_box + num_xboxes*20
        else:
            out_of_xbox = avg_x + 125
            if abs(out_of_xbox) > 0:
                num_xboxes = ceil(abs(out_of_xbox)/250)
            else:
                num_xboxes = 0
            new_xbox = launch_rail_box - num_xboxes*20
    else:
        num_xboxes = 0
        new_xbox = launch_rail_box
    
    return new_xbox


def update_yboxes(avg_y, launch_rail_box):
    ft = 3.2884
    avg_y *= ft
    
    if abs(avg_y) > 125:
        if avg_y > 0:
            out_of_ybox = avg_y - 125
            if abs(out_of_ybox) > 0:
                num_yboxes = ceil(out_of_ybox/250)
            else:
                num_yboxes = 0
            new_ybox = launch_rail_box + num_yboxes
        else:
            out_of_ybox = avg_y + 125
            if abs(out_of_ybox) > 0:
                num_yboxes = ceil(abs(out_of_ybox)/250)
            else:
                num_yboxes = 0
            new_ybox = launch_rail_box - num_yboxes
    else:
        num_yboxes = 0
        new_ybox = launch_rail_box
    
    return new_ybox


def oz_ascent_model2(w_0, imu_alt, imu_t_flight, my_max_sim_time=90, my_theta=2*pi/180, my_ssm=3.2, my_dry_base=15.89, my_m_motor=0.773, my_t_burn=1.57, my_T_avg=1000):
    P_0 = 101.325
    T_0 = 288.15
    R = 287
    B = 6.5*10**-3
    g = 9.80665  # gravity at sea level in m/s2

    imu_N = len(imu_alt)

    ## CONSTANT WIND PROFILE & AIR DENSITY FOR ENTIRE FLIGHT
    z_0 = max(imu_alt)  # apogee altitude in m

    wind_profile_x = np.zeros(imu_N);
    density_profile = np.zeros(imu_N);

    for ii in range(imu_N):
        T = T_0 - B*imu_alt[ii];
        P = P_0*1000*(T/T_0)**(g/(R*B));
        density_profile[ii] = P/(R*T);
        if imu_alt[ii] < 2:
            wind_profile_x[ii] = w_0*((2/z_0)**(1/7));
        else:
            wind_profile_x[ii] = w_0*((imu_alt[ii]/z_0)**(1/7))

    pi = 3.1415
    
    ## LAUNCH DAY INPUTS
    ###########################################################
    theta_0 = my_theta  # launch angle array in radians
    SSM = my_ssm  # static stability margin
    m_dry_base = my_dry_base
    ###########################################################
    
    ## UPDATE THESE BEFORE FULLSCALE
    ###########################################################
    T_avg = my_T_avg  # (1056) average motor thrust in N %change this based on apogee
    t_burn = my_t_burn  # motor burn time in s
    m_motor = my_m_motor  # motor propellant mass in kg
    ###########################################################
    
    m_dry = m_dry_base - m_motor  # rocket dry mass in kg
    number_of_time_steps = 2;

    ## MODEL #2: SIMULATE TRAJECTORY AFTER TAKE OFF
    max_sim_time = my_max_sim_time  # maximum simulation time in s
    dt = 0.001
    t = np.arange(0, max_sim_time, dt)  # time array
    N = len(t)  # time array size
    z = np.zeros(N)
    x = np.zeros(N)  # z and x displacement array
    vz = np.zeros(N)
    vx = np.zeros(N)  # z and x velocity array
    az = np.zeros(N)
    ax = np.zeros(N)  # z and x acceleration array
    v = np.zeros(N);
    m = np.zeros(N)  # mass array
    theta = np.zeros(N)  # angle array
    omega = np.zeros(N)  # angle array
    alpha = np.zeros(N)  # angle array

    # RAW PARAMETERS
    Cd = 0.39  # rocket drag coefficient
    Cd_side = 1  # rocket side drag coefficient
    L = 2.06  # rocket length in m
    D = 0.1524  # rocket diameter in m
    L_rail = 2  # launch rail transit in m

    # DERIVED PARAMETERS
    A_rocket = pi*(D**2)/4  # rocket cross sectional area in m2
    A_side_r = 0.3741  # rocket side area in m2
    m_wet = m_dry + m_motor  # rocket wet mass in kg
    m_dot = m_motor/t_burn  # motor burn rate in kg/s

    # SIMULATION PARAMETERS
    i = 1  # loop index
    z[i] = 0
    x[i] = 0  # initial displacement
    vz[i] = 0
    vx[i] = 0  # initial velocity
    m[i] = m_wet  # initial wet mass in kg
    ax[i] = T_avg/m[i]*sin(theta_0);
    az[i] = T_avg/m[i]*cos(theta_0) - g  # initial acceleration
    theta[i] = theta_0  # initial angle (launch) in radians
    i = i + 1  # increase loop

    ## STAGE 1: POWERED ASCENT ON LAUNCH RAIL
    # while z altitude is lower than the launch rail altitude
    while (np.linalg.norm((x[i-1], z[i-1])) < L_rail):

        theta[i] = theta_0  # constant angle until launch rail is cleared

        x[i] = x[i-1] + vx[i-1]*dt  # calculate x position
        z[i] = z[i-1] + vz[i-1]*dt  # calculate z position

        vz[i] = vz[i-1] + az[i-1]*dt  # calculate z velocity
        vx[i] = vx[i-1] + ax[i-1]*dt  # calculate x velocity
        v[i] = np.linalg.norm((vx[i], vz[i]))  # calculate velocity along axis

        m[i] = m[i-1] - m_dot*dt  # calculate mass

        ax[i] = T_avg/m[i]*sin(theta_0);
        az[i] = T_avg/m[i]*cos(theta_0) - g;

        i = i + 1  # increase simulation step

    t_LRE = t[i-1]  # launch rail exit time
    i_LRE = i -1;

    ## STAGE 2: WIND COCKING DURING POWERED ASCENT
    #[minDistance, imu_LRE] = min(abs(imu_alt - z[i_LRE]));
    minDistance = np.amin(abs(imu_alt - z[i_LRE]))
    imu_LRE = np.where(abs(imu_alt - z[i_LRE]) == minDistance)[0][0]

    w_LRE = abs(wind_profile_x[imu_LRE]);
    tau_ascent = (w_LRE/(T_avg/m[i_LRE]-g))*(SSM**2/(SSM-1));

    # while wind cocking occurs
    while (t[i-1] < t[i_LRE]+number_of_time_steps*tau_ascent):

        theta[i] = theta[i-1] + omega[i-1]*dt  # calculate angle
        omega[i] = omega[i-1] + alpha[i-1]*dt  # calculate angular velocity

        x[i] = x[i-1] + vx[i-1]*dt  # calculate x position
        z[i] = z[i-1] + vz[i-1]*dt  # calculate z position

        vz[i] = vz[i-1] + az[i-1]*dt  # calculate z velocity
        vx[i] = vx[i-1] + ax[i-1]*dt  # calculate x velocity
        v[i] = np.linalg.norm((vx[i], vz[i]))    

        #[minDistance, imu_index] = min(abs(imu_alt - z[i]));
        minDistance = np.amin(abs(imu_alt - z[i]))
        imu_index = np.where(abs(imu_alt - z[i]) == minDistance)[0][0]
        w = abs(wind_profile_x[imu_index])  # side wind calculation
        rho = density_profile[imu_index];

        m[i] = m[i-1] - m_dot*dt  # calculate mass

        I = 1/12*m[i]*(L**2)  # calculate inertia

        FD_side = 0.5*Cd_side*A_side_r*rho*((vx[i]+w)**2)  # calculate side drag
        FD = 0.5*Cd*rho*(v[i]**2)*A_rocket  # calculate drag along axis

        # calculate acceleration along rocket axis
        dv = (((T_avg-FD-FD_side*sin(theta[i]))/m[i])-g*cos(theta[i]))*dt;
        v[i] = v[i-1] + dv;

        vx[i] = v[i]*sin(theta[i]);
        vz[i] = v[i]*cos(theta[i]);

        # accelerations
        ax[i] = (dv/dt)*sin(theta[i]);
        az[i] = (dv/dt)*cos(theta[i]);
        alpha[i] = FD_side*SSM*D*cos(theta[i])/I;

        i = i + 1  # increase simulation step

    ## STAGE 3: POWERED ASCENT
    # while MECO is not reached
    while t[i-1] < t_burn:

        x[i] = x[i-1] + vx[i-1]*dt  # calculate x position
        z[i] = z[i-1] + vz[i-1]*dt  # calculate z position

        #[minDistance, imu_index] = min(abs(imu_alt - z[i]));
        minDistance = np.amin(abs(imu_alt - z[i]))
        imu_index = np.where(abs(imu_alt - z[i]) == minDistance)[0][0]
        rho = density_profile[imu_index];

        vz[i] = vz[i-1] + az[i-1]*dt  # calculate z velocity
        vx[i] = vx[i-1] + ax[i-1]*dt  # calculate x velocity
        v = sqrt((vz[i])**2 + (vx[i])**2)  # calculate velocity along axis

        theta[i] = atan(vx[i]/vz[i])  # calculate angle

        FD = 0.5*Cd*rho*(v**2)*A_rocket  # calculate drag along axis

        m[i] = m[i-1] - m_dot*dt  # calculate mass

        ax[i] = (T_avg-FD)*sin(theta[i])/m[i]  # calculate x accel.
        az[i] = (T_avg-FD)*cos(theta[i])/m[i]-g  # calculate y accel.

        i = i + 1  # increase simulation step

    ## STAGE 4: COAST ASCENT
    while (vz[i-1] > 0):
        x[i] = x[i-1] + vx[i-1]*dt  # calculate x position
        z[i] = z[i-1] + vz[i-1]*dt  # calculate z position

        #[minDistance, imu_index_111] = min(abs(imu_alt - z[i]));
        minDistance = np.amin(abs(imu_alt - z[i]))
        imu_index_111 = np.where(abs(imu_alt - z[i]) == minDistance)[0][0]
        rho = density_profile[imu_index_111];

        vz[i] = vz[i-1] + az[i-1]*dt  # calculate z velocity
        vx[i] = vx[i-1] + ax[i-1]*dt  # calculate x velocity
        v = sqrt((vz[i])**2 + (vx[i])**2)  # calculate velocity along axis

        theta[i] = atan(vx[i]/vz[i])  # calculate angle

        FD = 0.5*Cd*rho*(v**2)*A_rocket  #calculate drag along axis

        ax[i] = -FD*sin(theta[i])/m_dry  # calculate x accel.
        az[i] = -FD*cos(theta[i])/m_dry-g  # calculate y accel.

        i = i + 1  # increase simulation step
        
    x = [val for val in x if val!=0]
    if len(x)==0:
        x = [0]
    
    return x


def atan(ratio):
    # THIS IS IN RADIANS
    return atan2(ratio, 1)
