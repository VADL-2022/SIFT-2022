#include <python3.7m/Python.h>
#include <iostream>
#include <pigpio.h>
#include "IMU.hpp"
#include "config.hpp"
#include "VADL.hpp"
#include "LOG.hpp"
#include "IMU.hpp"
#include "LIDAR.hpp"
#include "LDS.hpp"
#include "MOTOR.hpp"

using namespace std;

VADL2022::VADL2022()
{
	cout << "Main: Initiating" << endl;

	connect_GPIO();
	connect_Python();
	mImu = new IMU();
	// mLidar = new LIDAR();
	// mLds = new LDS();
	// mMotor = new MOTOR();
	mLog = new LOG(nullptr, mImu, nullptr /*mLidar*/, nullptr /*mLds*/);

	mImu->receive();
	mLog->receive();

	cout << "Main: Initiated" << endl;
}

VADL2022::~VADL2022()
{
	cout << "Main: Destorying" << endl;

	delete (mLog);
	// delete (mMotor);
	// delete (mLds);
	// delete (mLidar);
	delete (mImu);

	cout << "Main: Destoryed" << endl;
}

void VADL2022::connect_GPIO()
{
	cout << "GPIO: Connecting" << endl;

	if (gpioInitialise() < 0)
	{
		cout << "GPIO: Failed to Connect" << endl;
		exit(1);
	}

	cout << "GPIO: Connected" << endl;
}

void VADL2022::disconnect_GPIO()
{
	cout << "GPIO: Disconnecting" << endl;

	gpioTerminate();

	cout << "GPIO: Disconnected" << endl;
}

void VADL2022::connect_Python()
{
	cout << "Python: Connecting" << endl;

	Py_Initialize();

	cout << "Python: Connected" << endl;
}

void VADL2022::disconnect_Python()
{
	cout << "Python: Disconnecting" << endl;

	Py_Finalize();

	cout << "Python: Disconnected" << endl;
}
