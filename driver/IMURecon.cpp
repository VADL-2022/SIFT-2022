#include <pybind11/pybind11.h>
namespace py = pybind11;

#include "VADL2022.hpp"
#include "subscaleMain.hpp"
#include "pyMainThreadInterface.hpp"
#include "py.h"
#include "../common.hpp"

void enqueueIMURecon(VADL2022* v) {
  mainDispatchQueue.enqueue([=](){
    //PyGILState_STATE state = PyGILState_Ensure(); // Only run this if no other python code is running now, otherwise wait for a lock // TODO: implement properly

    reportStatus(Status::RunningPython);
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
    const bool USE_DEFAULT_ARGS = false;
    using namespace pybind11::literals; // to bring in the `_a` literal
    py::list gridBoxNumbers = IMURecon.attr("calc_displacement2")(logFileName, USE_DEFAULT_ARGS ? ("launch_rail_box"_a="192") : ("launch_rail_box"_a=v->launchBox), "my_thresh"_a="50", "my_post_drogue_delay_a"="0.85", "my_signal_length"_a="3", "my_t_sim_landing"_a="50", USE_DEFAULT_ARGS ? ("ld_launch_angle"_a="2*pi/180") : ("ld_launch_angle"_a=v->launchAngle), "ld_ssm"_a="3.2", "ld_dry_base"_a="15.89");
    py::print("C++ got gridBoxNumbers:", gridBoxNumbers);

    // Send the gridBoxNumbers on the radio
    const char *sendOnRadioScriptArgs[] = {NULL, NULL};
    sendOnRadioScriptArgs[0] = "0"; // 1 to use stdin
    std::string gridBoxNumbers_str = py::str(gridBoxNumbers); // Cast the python list to std::string
    sendOnRadioScriptArgs[1] = gridBoxNumbers_str.c_str(); // String to send
    //sendOnRadioScriptArgs[2] = ""; // Send this file on the radio
    { out_guard();
      std::cout << "sendOnRadio script execution" << std::endl; }
    bool success = S_RunFile("driver/radio.py", 2, (char **)sendOnRadioScriptArgs);

    //PyGILState_Release(state); // TODO: implement properly
  },"IMURecon.py",QueuedFunctionType::Python);
}
