#include <iostream>
#include <pigpio.h>
#include "config.hpp"
#include "IMU.hpp"
#include "../commonOutMutex.hpp"

using namespace std;
using namespace vn::protocol::uart;

void IMU::callback(void *userData, Packet &asyncPacket, size_t packetStartRunningIndex)
{
	IMU *data = (IMU *)userData;

	if (asyncPacket.type() == Packet::TYPE_BINARY)
	{
		if (!asyncPacket.isCompatible(
				IMU_OUTPUT.commonField,
				IMU_OUTPUT.timeField,
				IMU_OUTPUT.imuField,
				IMU_OUTPUT.gpsField,
				IMU_OUTPUT.attitudeField,
				IMU_OUTPUT.insField,
				IMU_OUTPUT.gps2Field))
		{
			return;
		}

		data->timestamp = asyncPacket.extractUint64();		// Nanoseconds
		data->yprNed = asyncPacket.extractVec3f();			// Degrees
		data->qtn = asyncPacket.extractVec4f();				// Quaternion
		data->rate = asyncPacket.extractVec3f();			// Rad per Second
		data->accel = asyncPacket.extractVec3f();			// Meters per Second Squared
		data->mag = asyncPacket.extractVec3f();				// Gauss
		data->temp = asyncPacket.extractFloat();			// Celsius
		data->pres = asyncPacket.extractFloat();			// Kilopascals
		data->dTime = asyncPacket.extractFloat();			// Seconds
		data->dTheta = asyncPacket.extractVec3f();			// Degrees per Second
		data->dVel = asyncPacket.extractVec3f();			// Meters per Second Squared
		data->magNed = asyncPacket.extractVec3f();			// Gauss
		data->accelNed = asyncPacket.extractVec3f();		// Meters per Second Squared
		data->linearAccelBody = asyncPacket.extractVec3f(); // Meters per Second Squared
		data->linearAccelNed = asyncPacket.extractVec3f();	// Meters per Second Squared
	}
}

IMU::IMU()
{
	if (IMU_ACTIVE)
	{
		{ out_guard();
		  cout << "IMU: Connecting" << endl; }

		mImu.connect(IMU_PORT, IMU_BAUD_RATE);

		if (!mImu.isConnected())
		{
			{ out_guard();
			  cout << "IMU: Failed to Connect" << endl; }
			throw "IMU: Failed to Connect";
			//exit(1);
		}

		{ out_guard();
		  cout << "IMU: Connected" << endl; }
	}
}

IMU::~IMU()
{
	if (IMU_ACTIVE)
	{
		{ out_guard();
		  cout << "IMU: Disconnecting" << endl; }

		halt();
		mImu.disconnect();

		{ out_guard();
		  cout << "IMU: Disconnected" << endl; }
	}
}

void IMU::reset()
{
	if (IMU_ACTIVE)
	{
		{ out_guard();
		  cout << "IMU: Resetting" << endl; }

		mImu.restoreFactorySettings();
		gpioSleep(PI_TIME_RELATIVE, 2, 0);

		{ out_guard();
		  cout << "IMU: Resetted" << endl; }
	}
}

void IMU::receive()
{
	if (IMU_ACTIVE)
	{
		{ out_guard();
		  cout << "IMU: Initiating Receiver" << endl; }

		mImu.writeBinaryOutput1(IMU_OUTPUT);
		mImu.registerAsyncPacketReceivedHandler(this, callback);

		{ out_guard();
		  cout << "IMU: Initiated Receiver" << endl; }
	}
}

void IMU::halt()
{
	if (IMU_ACTIVE)
	{
		{ out_guard();
		  cout << "IMU: Destroying Receiver" << endl; }

		mImu.unregisterAsyncPacketReceivedHandler();

		{ out_guard();
		  cout << "IMU: Destroyed Receiver" << endl; }
	}
}
