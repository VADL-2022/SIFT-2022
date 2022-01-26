#!/usr/bin/env python
# coding: utf-8

# In[2]:


import pandas as pd
import numpy as np
from math import sin, cos, pi, atan2, asin
import matplotlib.pyplot as plt
from scipy import integrate

from get_YPR_matrix import convert_YPR_fixed_frame


# In[4]:


def readCSV(filename):
    with open(filename) as csvDataFile:
        # Read file as csv
        df = pd.read_csv(csvDataFile)
    return df


def nasa_pres(P, P0=101.29, T0=288.08):
    T = T0*((P / P0)**(1 / 5.256)) - 273.1
    h = (T-15.04)/(-0.00649)
    return h


################## CONSTANTS ##################
# gravity
g = 9.81  # m/s^2
# m to ft
ft = 3.2884  # ft/m

################## DATA FRAME ##################
datafile = 'WalkTest.csv'
fields = ['Timestamp', 'Pres',
  'Roll', 'Pitch', 'Yaw',
  'LinearAccelNed X', 'LinearAccelNed Y', 'LinearAccelNed Z'
  ]

df = pd.read_csv(datafile, skipinitialspace=True, usecols=fields)

################## INIT VECTORS ##################
all_time = df['Timestamp'].values

tdata = all_time
tdata = tdata - all_time[0]


# In[5]:


ax_vn = df['LinearAccelNed X'] * ft
ay_vn = df['LinearAccelNed Y'] * ft
az_vn = df['LinearAccelNed Z'] * -ft

vx_vn, vy_vn, vz_vn = np.zeros(len(ax_vn)),np.zeros(len(ay_vn)), np.zeros(len(az_vn))
x_vn, y_vn, z_vn = np.zeros(len(ax_vn)),np.zeros(len(ay_vn)), np.zeros(len(az_vn))
dt = tdata[1]

vx_vn = integrate.cumtrapz(ax_vn, tdata, initial=0)
vy_vn = integrate.cumtrapz(ay_vn, tdata, initial=0)
vz_vn = integrate.cumtrapz(az_vn, tdata, initial=0)

x_vn = integrate.cumtrapz(vx_vn, tdata, initial=0)
y_vn = integrate.cumtrapz(vy_vn, tdata, initial=0)
z_vn = integrate.cumtrapz(vz_vn, tdata, initial=0)


# In[6]:


fig1 = plt.figure(1, figsize=(8,8))
ax = plt.subplot(111)
l2 = ax.plot(tdata, ax_vn, color='red', label='x')
l3 = ax.plot(tdata, ay_vn, color='green', label='y')
l4 = ax.plot(tdata, az_vn, color='purple', label='z')
l5 = plt.axhline(y=0, color='black', linewidth=2)
ax.set_title("LinAccelNed From VN")
ax.set_xlabel('Time (s)')
ax.set_ylabel("Acceleration [ft/s2]")
ax.legend()
plt.show()


# In[7]:


fig1 = plt.figure(1, figsize=(8,8))
ax = plt.subplot(111)
l2 = ax.plot(tdata, vx_vn, color='red', label='x')
l3 = ax.plot(tdata, vy_vn, color='green', label='y')
l4 = ax.plot(tdata, vz_vn, color='purple', label='z')
l5 = plt.axhline(y=0, color='black', linewidth=2)
ax.set_title("Velocity Integrated From VN")
ax.set_xlabel('Time (s)')
ax.set_ylabel("Velocity [ft/s]")
ax.legend()
plt.show()


# In[8]:


fig1 = plt.figure(1, figsize=(8,8))
# Just the Displacement:
ax = plt.subplot(111)
#l1 = ax.plot(tdata, altitude_NASA_pres, color='blue', label='NASA Pressure')
l2 = ax.plot(tdata, x_vn, color='red', label='x')
l3 = ax.plot(tdata, y_vn, color='green', label='y')
l4 = ax.plot(tdata, z_vn, color='purple', label='z')
l5 = plt.axhline(y=0, color='black', linewidth=2)
ax.set_title("Altitude From Numerical Integration")
ax.set_xlabel('Time (s)')
ax.set_ylabel("Altitude [ft]")
ax.legend()
plt.show()


# ## Better Fitting

# In[43]:


start = 275
end = 520

fitted_ax, fitted_ay,  fitted_az = ax_vn[start:end], ay_vn[start:end], az_vn[start:end]
fitted_time = tdata[start:end]

vx_vn, vy_vn, vz_vn = np.zeros(len(fitted_ax)),np.zeros(len(fitted_ax)), np.zeros(len(fitted_ax))
x_vn, y_vn, z_vn = np.zeros(len(fitted_ax)),np.zeros(len(fitted_ax)), np.zeros(len(fitted_ax))
dt = tdata[1]

fitted_vx = integrate.cumtrapz(fitted_ax, fitted_time, initial=0)
fitted_vy = integrate.cumtrapz(fitted_ay, fitted_time, initial=0)
fitted_vz = integrate.cumtrapz(fitted_az, fitted_time, initial=0)

fitted_x = integrate.cumtrapz(fitted_vx, fitted_time, initial=0)
fitted_y = integrate.cumtrapz(fitted_vy, fitted_time, initial=0)
fitted_z = integrate.cumtrapz(fitted_vz, fitted_time, initial=0)


# In[44]:


fig1 = plt.figure(1, figsize=(8,8))
ax = plt.subplot(111)
l2 = ax.plot(tdata[start:end], ax_vn[start:end], color='red', label='x')
l3 = ax.plot(tdata[start:end], ay_vn[start:end], color='green', label='y')
l4 = ax.plot(tdata[start:end], az_vn[start:end], color='purple', label='z')
l5 = plt.axhline(y=0, color='black', linewidth=2)
ax.set_title("LinAccelNed From VN")
ax.set_xlabel('Time (s)')
ax.set_ylabel("Acceleration [ft/s2]")
ax.legend()
plt.show()


# In[45]:


fig1 = plt.figure(1, figsize=(8,8))
ax = plt.subplot(111)
l2 = ax.plot(fitted_time, fitted_vx, color='red', label='x')
l3 = ax.plot(fitted_time, fitted_vy, color='green', label='y')
l4 = ax.plot(fitted_time, fitted_vz, color='purple', label='z')
l5 = plt.axhline(y=0, color='black', linewidth=2)
ax.set_title("Velocity Integrated From VN")
ax.set_xlabel('Time (s)')
ax.set_ylabel("Velocity [ft/s]")
ax.legend()
plt.show()


# In[46]:


fig1 = plt.figure(1, figsize=(8,8))
# Just the Displacement:
ax = plt.subplot(111)
#l1 = ax.plot(tdata, altitude_NASA_pres, color='blue', label='NASA Pressure')
l2 = ax.plot(fitted_time, fitted_x, color='red', label='x')
l3 = ax.plot(fitted_time, fitted_y, color='green', label='y')
l4 = ax.plot(fitted_time, fitted_z, color='purple', label='z')
l5 = plt.axhline(y=0, color='black', linewidth=2)
l5 = plt.axhline(y=-33, color='black', linewidth=2)
ax.set_title("Altitude From Numerical Integration")
ax.set_xlabel('Time (s)')
ax.set_ylabel("Altitude [ft]")
ax.legend()
#ax.set_ylimit(-35)
plt.show()


# In[ ]:




