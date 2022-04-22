// g++ -I VectorNav/include ctest.cpp
#include <stdio.h>
#include "src/IMUData.hpp"
#include <iostream>

int main() {
  FILE* f = fopen("Data/fullscale3/sift2/LOG_20220416_114053.csv", "r");
  printf("%p\n",f);
  char buf[1000];
  //printf("%d\n", fscanf(f, "%s", buf)); // UNSAFE due to arbitrary long string
  //printf("%s\n",buf);
  printf("%d\n", fscanf(f, "Timestamp,Yaw,Pitch,Roll,Qtn[0], Qtn[1], Qtn[2], Qtn[3],Rate X,Rate Y,Rate Z,Accel X,Accel Y,Accel Z,Mag X,Mag Y,Mag Z,Temp,Pres,dTime,dTheta X,dTheta Y,dTheta Z,dVel X,dVel Y,dVel Z,MagNed X,MagNed Y,MagNed Z,AccelNed X,AccelNed Y,AccelNed Z,LinearAccel X,LinearAccel Y,LinearAccel Z,LinearAccelNed X,LinearAccelNed Y,LinearAccelNed Z,Amplitude,Distance,Light 1,Light 2,Light 3,Light 4,Comments\n"));
  IMUData imu;
  for (int i = 0; i < 10; i++) {
  float timestampSeconds;
       // printf("%d\n", fscanf(f,
       //                       "%f", &timestampSeconds));
      fscanf(f,
           "%f"
           ",%f,%f,%f" // yprNed
           ",%f,%f,%f,%f" // qtn
           ",%f,%f,%f" // rate
           ",%f,%f,%f" // accel
           ",%f,%f,%f" // mag
           ",%f,%f,%f" // temp,pres,dTime
           ",%f,%f,%f" // dTheta
           ",%f,%f,%f" // dVel
           ",%f,%f,%f" // magNed
           ",%f,%f,%f" // accelNed
           ",%f,%f,%f" // linearAccelBody
           ",%f,%f,%f" // linearAccelNed
           "," // Nothing after this comma on purpose.
           ,
           &timestampSeconds,
           &imu.yprNed.x, &imu.yprNed.y, &imu.yprNed.z,
           &imu.qtn.x, &imu.qtn.y, &imu.qtn.z, &imu.qtn.w,
           &imu.rate.x, &imu.rate.y, &imu.rate.z,
           &imu.accel.x, &imu.accel.y, &imu.accel.z,
           &imu.mag.x, &imu.mag.y, &imu.mag.z,
           &imu.temp, &imu.pres, &imu.dTime,
           &imu.dTheta.x, &imu.dTheta.y, &imu.dTheta.z,
           &imu.dVel.x, &imu.dVel.y, &imu.dVel.z,
           &imu.magNed.x, &imu.magNed.y, &imu.magNed.z,
           &imu.accelNed.x, &imu.accelNed.y, &imu.accelNed.z,
           &imu.linearAccelBody.x, &imu.linearAccelBody.y, &imu.linearAccelBody.z,
           &imu.linearAccelNed.x, &imu.linearAccelNed.y, &imu.linearAccelNed.z
           );
    imu.timestamp = timestampSeconds * 1e9; // Convert to nanoseconds
    std::cout << timestampSeconds << ", " << imu.timestamp << ", " << imu.yprNed.x << std::endl;
  }

  return 0;
}
