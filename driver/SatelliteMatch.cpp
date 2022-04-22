#include "SatelliteMatch.hpp"

#include <pybind11/eval.h>
#include <pybind11/pybind11.h>
namespace py = pybind11;

#include "VADL2022.hpp"
#include "subscaleMain.hpp"
#include "pyMainThreadInterface.hpp"
#include "py.h"
#include "../common.hpp"
#include "../pythonCommon.hpp"

// Returns a non-empty string if successful, otherwise it's empty
std::string getOutputMatrix() {
  // https://stackoverflow.com/questions/646241/c-run-a-system-command-and-get-output
  FILE *fp;
  char path[1035]; // Note: hack, but names won't be longer than a known amount anyway
  /* Open the command for reading. */
  const char* cmd_ = "ls -td dataOutput/*/ | head -1";
  printf("Running: %s\n", cmd_);
  fp = popen(cmd_, "r");
  if (fp == NULL) {
    printf("Failed to run ls command.\n" );
    return "";
  }
  /* Read the output a line at a time. */
  std::string outputAcc2 = "";
  while (fgets(path, sizeof(path)*sizeof(char), fp) != NULL) {
    outputAcc2 += path;
  }
  // Remove newline if any
  size_t len = outputAcc2.size();
  if (len > 1 && outputAcc2[len-1] == '\n') {
    outputAcc2.pop_back();            //[len-1] = '\0'; <-- THIS CAUSES PROBLEMS with appending to the string!
  }
  /* close */
  pclose(fp);

  { out_guard();
    std::cout << "Matrix directory: " << outputAcc2 << std::endl; }

  // FIXME: HACK:
  char buf[10000];
  sprintf(buf, "./compareNatSort/compareNatSort '%s' '%s' | tail -1", outputAcc2.c_str(), ".matrix0.txt");
  
  // printf("Running: %s\n", cmd.c_str());
  // fp = popen(cmd.c_str(), "r");
  printf("Running: %s\n", buf);
  fp = popen(buf, "r");
  if (fp == NULL) {
    printf("Failed to run matrix compareNatSort command.\n" );
    return "";
  }
  /* Read the output a line at a time. */
  //outputAcc.clear();
  while (fgets(path, sizeof(path)*sizeof(char), fp) != NULL) {
    // Grab until last line (do nothing)
  }
  // Remove newline if any
  len = strlen(path);
  if (len > 1 && path[len-1] == '\n') {
    path[len-1] = '\0';
  }
  std::string outputAcc_ = path; // Get last line
  /* close */
  pclose(fp);

  { out_guard();
    std::cout << "getOutputMatrix(): file: " << outputAcc_ << std::endl; }
  return outputAcc_;
}

std::string getOutputFirstImage() {
  // https://stackoverflow.com/questions/646241/c-run-a-system-command-and-get-output
  FILE *fp;
  char path[1035]; // Note: hack, but names won't be longer than a known amount anyway
  /* Open the command for reading. */
  const char* cmd_ = "ls -td dataOutput/*/ | head -1";
  printf("Running: %s\n", cmd_);
  fp = popen(cmd_, "r");
  if (fp == NULL) {
    printf("Failed to run ls command.\n" );
    return "";
  }
  /* Read the output a line at a time. */
  std::string outputAcc2 = "";
  while (fgets(path, sizeof(path)*sizeof(char), fp) != NULL) {
    outputAcc2 += path;
  }
  // Remove newline if any
  size_t len = outputAcc2.size();
  if (len > 1 && outputAcc2[len-1] == '\n') {
    outputAcc2.pop_back();            //[len-1] = '\0'; <-- THIS CAUSES PROBLEMS with appending to the string!
  }
  /* close */
  pclose(fp);

  { out_guard();
    std::cout << "First image directory: " << outputAcc2 << std::endl; }

  return outputAcc2 + "/firstImage0.png";
}

void enqueueSatelliteMatch(VADL2022* v) {
  auto enq = [=](){
    nonthrowing_python_nolock([=](){
      std::string matrixFilename = getOutputMatrix();
      std::string firstImageFilename = getOutputFirstImage();

      py::module_ match = py::module_::import("driver.satelliteImageMatching");
      py::int_ gridIdentifier = (py::tuple)(match.attr("run")(matrixFilename, firstImageFilename, 640/2, 480/2))[0]; // HACK: hardcoded 480p

      std::cout << "grid@@@@@@@@@@@@@@@@@@@@@" << std::endl;
    });
  };
  mainDispatchQueue.enqueue(enq, "satelliteImageMatching.py",QueuedFunctionType::Python);
}
