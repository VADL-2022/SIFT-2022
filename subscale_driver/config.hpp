#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "../vn_sensors_common.hpp"
//#include "LdSensorOneModbus.h"

using namespace std;
using namespace vn::sensors;
using namespace vn::protocol::uart;
//using namespace LeddarConnection;

const uint16_t FREQUENCY = 40;                        // Hertz
const bool TEST = 0;                                  // Boolean
const bool VERBOSE = TEST ? 0 : 0;                    // Boolean
const bool LOG_ACTIVE = TEST ? 0 : 1;                 // Boolean
const bool COMMS_ACTIVE = TEST ? 1 : 1;               // Boolean
const bool IMU_ACTIVE = TEST ? 0 : 1;                 // Boolean
const bool LIDAR_ACTIVE = TEST ? 0 : 1;               // Boolean
const bool LDS_ACTIVE = TEST ? 0 : 1;                 // Boolean
const bool MOTOR_ACTIVE = TEST ? 0 : 1;               // Boolean
const bool PDS_ACTIVE = TEST ? 0 : 1;                 // Boolean
const bool IS_ACTIVE = TEST ? 0 : 1;                  // Boolean
const uint16_t GDS_JETTISON_COOLDOWN = TEST ? 2 : 10; // Seconds
const uint16_t GDS_RELEASE_TIMEOUT = TEST ? 10 : 300; // Seconds
const uint16_t GDS_MOCK_LDS_TIMER = TEST ? 0 : 60;    // Seconds
const uint8_t GDS_LDS_THREASHHOLD = 2;                // Count
const float PDS_ACTIVATION_TIME = 0.2;                // Seconds
const float PDS_DELAY_TIME = 1;                       // Seconds
const float GDS_LANDING_IMPACT_THRESHOLD = 50;        // Meters per Second Squared
const float GDS_LANDING_ALTITUDE_THRESHOLD = 0.25;    // Meter
const float LS_I_GAIN = 0.5;                          // Gain
const float LS_ANGLE_THRESHOLD = 2.5;                 // Degree
const float LS_STABILITY_THRESHOLD = 5;               // Seconds
const string LOG_FILE = "./dataOutput/LOG";                  // File Path
const uint8_t LOG_TIMER = 0;                          // Timer
const string IMG_FILE = "./img/IMG";                  // File Path
const string IMU_PORT = "/dev/ttyUSB0";               // Port
const uint32_t IMU_BAUD_RATE = 115200;                // Baud
static BinaryOutputRegister IMU_OUTPUT(               // IMU Output
    ASYNCMODE_PORT1,
    800 / FREQUENCY,
    COMMONGROUP_TIMESTARTUP | COMMONGROUP_YAWPITCHROLL | COMMONGROUP_QUATERNION | COMMONGROUP_ANGULARRATE | COMMONGROUP_ACCEL | COMMONGROUP_MAGPRES | COMMONGROUP_DELTATHETA,
    TIMEGROUP_NONE,
    IMUGROUP_NONE,
    GPSGROUP_NONE,
    ATTITUDEGROUP_MAGNED | ATTITUDEGROUP_ACCELNED | ATTITUDEGROUP_LINEARACCELBODY | ATTITUDEGROUP_LINEARACCELNED,
    INSGROUP_NONE,
    GPSGROUP_NONE);
const string LIDAR_PORT = "/dev/ttyUSB1";                                                    // Port
const uint32_t LIDAR_BAUD_RATE = 115200;                                                     // Baud
//const LdConnectionInfoModbus::eParity LIDAR_PARITY = LdConnectionInfoModbus::MB_PARITY_NONE; // Bits
const uint8_t LIDAR_DATA_BITS = 8;                                                           // Bits
const uint8_t LIDAR_STOP_BITS = 1;                                                           // Bits
const uint8_t LIDAR_MODBUS_ADDRESS = 1;                                                      // Bits
const uint8_t LIDAR_TIMER = 1;                                                               // Timer
const uint8_t LDS_PIN_1 = 2;                                                                 // Pin
const uint8_t LDS_PIN_2 = 3;                                                                 // Pin
const uint8_t LDS_PIN_3 = 4;                                                                 // Pin
const uint8_t LDS_PIN_4 = 17;                                                                // Pin
const uint8_t PDS_PIN = 26;                                                                  // Pin
const uint8_t IS_PIN = 19;                                                                   // Pin
const uint8_t MOTOR_COUNT = 4;                                                               // Count
const uint8_t MOTOR_BLUE = 0;                                                                // Enum
const uint8_t MOTOR_RED = 1;                                                                 // Enum
const uint8_t MOTOR_BLACK = 2;                                                               // Enum
const uint8_t MOTOR_YELLOW = 3;                                                              // Enum
const struct Motor                                                                           // Structure
{
    string SERIAL; // SERIAL
    string ODRV;   // Pin
    string AXIS;   // Pin
} MOTORS[MOTOR_COUNT] = {
    {"205B378F5753", "0", "0"}, // BLUE
    {"205B378F5753", "0", "1"}, // RED
    {"2071377E5753", "1", "0"}, // BLACK
    {"2071377E5753", "1", "1"}  // YELLOW
};

#endif
