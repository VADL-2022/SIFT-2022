# Trying to implement this paper: https://www.mdpi.com/2078-2489/9/12/299 -> https://www.mdpi.com/2078-2489/9/12/299/pdf

import numpy as np

# Gaussian
# x,y are points from image I(x,y)
def G(x,y,sigma):
    sigmaSquared=sigma**2
    return 1/(2*np.pi*sigmaSquared)*np.exp(-(x**2+y**2))/(2*sigmaSquared)

def r(theta):
    

def backProjectToSphere(x, y):
    r(theta)
