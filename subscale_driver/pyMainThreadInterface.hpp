#include "py.h"
#include "subscaleMain.hpp"
#include <mutex>

extern pid_t lastPyPID;
extern bool lastPyPIDValid;
extern std::mutex lastPyPIDM;

bool runCommandWithFork(const char* commandWithArgs[] /* array with NULL as the last element */) {
  const char* command = commandWithArgs[0];
  printf("Forking to run command \"%s\"\n", command);
  pid_t pid = fork(); // create a new child process
  if (pid > 0) { // Parent process
    lastPyPIDM.lock();
    lastPyPID=pid;
    lastPyPIDValid=true;
    lastPyPIDM.unlock();
    int status = 0;
    if (wait(&status) != -1) {
      if (WIFEXITED(status)) {
	// now check to see what its exit status was
	printf("The exit status for command \"%s\" was: %d\n", command, WEXITSTATUS(status));
      } else if (WIFSIGNALED(status)) {
	// it was killed by a signal
	printf("The signal that killed command \"%s\" was %d\n", command, WTERMSIG(status));
      }
    } else {
      printf("Error waiting for command \"%s\"!\n", command);
          lastPyPIDM.lock();
    lastPyPIDValid=false;
    lastPyPIDM.unlock();
      return false;
    }
        lastPyPIDM.lock();
    lastPyPIDValid=false;
    lastPyPIDM.unlock();
  } else if (pid == 0) { // Child process
    execvp((char*)command, (char**)commandWithArgs); // one variant of exec
    printf("Failed to run execvp to run command \"%s\": %s\n", command, strerror(errno)); // Will only print if error with execvp.
    return false;
  } else {
    printf("Error with fork for command \"%s\": %s\n", command, strerror(errno));
    return false;
  }
  return true;
}

inline bool pyRunFile(const char *path, int argc, char **argv) {
  mainDispatchQueue.enqueue([=](){
			      //S_RunFile(path, argc, argv);

			      
	mainDispatchQueue.enqueue([](){
				     const char* args[] = {"python3.7m", "subscale_driver/videoCapture.py", NULL};
				     if (!runCommandWithFork(args)) {
				       std::cout << "Running videoCapture with fork failed" << std::endl;
				     }
				  }, "videoCapture.py",QueuedFunctionType::Python);
	
  },path,QueuedFunctionType::Python);

  return true;
}
