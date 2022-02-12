//
//  SubscaleDriverInterface.hpp
//  SIFT
//
//  Created by VADL on 2/11/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#ifndef SubscaleDriverInterface_hpp
#define SubscaleDriverInterface_hpp

#include "../IMUData.hpp"
#include <mutex>

void* subscaleDriverInterfaceThreadFunc(void* arg);

// Guarded by subscaleDriverInterfaceMutex_imuData //
extern int subscaleDriverInterface_count; // How many lines of IMU data were grabbed (0 for none yet)
extern IMUData subscaleDriverInterface_imu;
// //
extern std::mutex subscaleDriverInterfaceMutex_imuData;

#endif /* SubscaleDriverInterface_hpp */
