#include "IMURecon.hpp"

#include <pybind11/eval.h>
#include <pybind11/pybind11.h>
namespace py = pybind11;

#include "VADL2022.hpp"
#include "subscaleMain.hpp"
#include "pyMainThreadInterface.hpp"
#include "py.h"
#include "../common.hpp"
#include "../pythonCommon.hpp"

void enqueueIMURecon(VADL2022* v) {
  auto enq = rec([=](auto&& enq){
    //PyGILState_STATE state = PyGILState_Ensure(); // Only run this if no other python code is running now, otherwise wait for a lock // TODO: implement properly
    
    nonthrowing_python_nolock([=](){
      reportStatus(Status::RunningPython);
      puts("((((((((((((((((((((((((((((((((((((((((");
      auto now = std::chrono::steady_clock::now();
      auto timeSinceLanding = now - v->landingTime;
      auto sleepTime = std::chrono::seconds(100) - timeSinceLanding;
      { out_guard();
        std::cout << "Time since landing: " << std::chrono::duration_cast<std::chrono::milliseconds>(timeSinceLanding).count() << " milliseconds" << std::endl; }
      if (v->landingTime > now) {
        { out_guard();
          std::cout << "sleepTime would be negative (overflow due to unsigned), no landing, so not sleeping! (Would have been for " << std::chrono::duration_cast<std::chrono::milliseconds>(sleepTime).count() << " milliseconds)" << std::endl; }
      }
      else if (!v->noIMUReconSleep) {
        { out_guard();
          std::cout << "Sleeping for " << std::chrono::duration_cast<std::chrono::milliseconds>(sleepTime).count() << " milliseconds" << std::endl; }
        std::this_thread::sleep_for(sleepTime);
      }
      else {
        { out_guard();
          std::cout << "Sleeping would have been for " << std::chrono::duration_cast<std::chrono::milliseconds>(sleepTime).count() << " milliseconds" << std::endl; }
      }
      puts("))))))))))))))))))))))))))))))))))))))))");
      py::module_ IMURecon = py::module_::import("driver.IMURecon");
      const char* logFileName;
      if (v->imuDataSourcePath == nullptr) {
	// Real IMU
	logFileName = ((LOG*)v->mLog)->mLogFileName.c_str();
      }
      else {
	// IMU is from a file
	logFileName = v->imuDataSourcePath;
      }
      using namespace pybind11::literals; // to bring in the `_a` literal
      py::tuple gridBoxNumbersAndOtherJunk = IMURecon.attr("calc_displacement2")(logFileName, py::arg("launch_rail_box")=py::eval(v->launchBox), "weather_station_stats_xy"_a=py::eval(v->windSpeed), "GPS_coords"_a=py::eval(v->launchRailGPSXYCoords), py::arg("ld_launch_angle")=py::eval(v->launchAngle));
      py::print("gridBoxNumbersAndOtherJunk:", gridBoxNumbersAndOtherJunk);
      //py::list gridBoxNumbers = gridBoxNumbersAndOtherJunk[0]; // The first model
      py::tuple gridBoxNumbers = gridBoxNumbersAndOtherJunk;
      py::print("C++ got gridBoxNumbers:", gridBoxNumbers);

      auto enq1 = rec([=](auto&& enq1){
        nonthrowing_python_nolock([=](){
          // Send the gridBoxNumbers on the radio
          const char *sendOnRadioScriptArgs[] = {NULL, NULL};
          sendOnRadioScriptArgs[0] = "0"; // 1 to use stdin
          std::string gridBoxNumbers_str = py::str(gridBoxNumbers); // Cast the python list to std::string
          sendOnRadioScriptArgs[1] = gridBoxNumbers_str.c_str(); // String to send
          //sendOnRadioScriptArgs[2] = ""; // Send this file on the radio
          { out_guard();
            std::cout << "sendOnRadio script execution for IMURecon" << std::endl; }
          bool success = S_RunFile("driver/radio.py", 2, (char **)sendOnRadioScriptArgs);
          
          // Enqueue a send again so we send on radio again in case of transmission error
          mainDispatchQueue.enqueue(enq1,"IMURecon.py enqueue again",QueuedFunctionType::Python);
        });
      });
        
      enq1();
    });

    //PyGILState_Release(state); // TODO: implement properly
  });
  
  mainDispatchQueue.enqueue(enq,"IMURecon.py",QueuedFunctionType::Python);
}
