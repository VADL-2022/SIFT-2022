//
//  IMUData.hpp
//  SIFT
//
//  Created by VADL on 1/20/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#ifndef IMUData_hpp
#define IMUData_hpp

#include "../vn_sensors_common.hpp"

using namespace vn::math;
using namespace vn::sensors;
using namespace vn::protocol::uart;

class IMUData
{
public:
    //VnSensor mImu;
    uint64_t timestamp = 0; // Nanoseconds
    float fseconds;
    vec3f yprNed;            // Degrees
    vec4f qtn;                // Quaternion
    vec3f rate;                // Rad per Second
    vec3f accel;            // Meters per Second Squared
    vec3f mag;                // Gauss
    float temp = 0;            // Celsius
    float pres = 0;            // Kilopascals
    float dTime = 0;        // Seconds
    vec3f dTheta;            // Degrees per Second
    vec3f dVel;                // Meters per Second Squared
    vec3f magNed;            // Gauss
    vec3f accelNed;            // Meters per Second Squared
    vec3f linearAccelBody;    // Meters per Second Squared
    vec3f linearAccelNed;    // Meters per Second Squared
};

#endif /* IMUData_hpp */
