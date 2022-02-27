#include "subscaleMain.hpp"

#include <python3.7m/Python.h>
#include "VADL2022.hpp"
#include <thread>
#include "py.h"
#ifdef THREAD_OUTPUT_TEST2
#include <mutex>
#endif
#include "../common.hpp"

//Queue<QueuedFunction, 8> mainDispatchQueue;
Queue<QueuedFunction, 1024> mainDispatchQueue;
Queue<QueuedFunction, 16> radioDispatchQueue;
std::atomic<bool> isRunningPython = false;
std::atomic<bool> mainDispatchQueueRunning = false;
std::atomic<bool> mainDispatchQueueDrainThenStop = false;
std::unique_ptr<std::thread> videoCaptureOnlyThread;

int main(int argc, char **argv) {
  #ifdef THREAD_OUTPUT_TEST // To use: run the driver with your command followed by `| grep -v a123` (`-v` for inverted match (match *not* what is given)) and ensure no output prints
  // This fails the test: std::thread t([](){while(1) std::cout << "a"<<1<<2<<3<<std::endl;});
  // This also fails the test likely because the endl is causing a flush via the second `<<` which happens after the first `<<`. It causes `asd123asd123\n\n` (where `\n` are actual newlines) to be printed sometimes: std::thread t1_1([](){while(1) std::cout << "a123"<<std::endl;});
  // These all pass the test:
  std::thread t1_2([](){while(1) std::cout << "a123\n";});
  std::thread t1([](){while(1)printf("a%d%d%d\n",1,2,3);});
  std::thread t2([](){while(1)printf("a%d%d%d\n",1,2,3);});
  std::thread t3([](){while(1)printf("a%d%d%d\n",1,2,3);});
  while(1){}
  #endif
  #ifdef THREAD_OUTPUT_TEST2
  int i;
  std::mutex m;
  std::thread t1_2([&]() {
    while (1) {
      m.lock();
      i = (i + 1) % 26;
      m.unlock();
      std::cout << (char)('a' + i);
    }
  });
  std::thread t1_3([&]() {
    while (1) {
      m.lock();
      i = (i + 1) % 9;
      m.unlock();
      std::cout << (char)('a' + i);
    }
  });
  std::thread t1_4([&]() {
    while (1) {
      m.lock();
      i = (i + 1) % 9;
      m.unlock();
      std::cout << (char)('a' + i);
    }
  });
  std::thread t1([&]() {
    while (1) {
      m.lock();
      i = (i + 1) % 9;
      m.unlock();
      printf("%c\n", 'a' + i);
    }
  });
  std::thread t2([&]() {
    while (1) {
      m.lock();
      i = (i + 1) % 9;
      m.unlock();
      printf("%c\n", 'a' + i);
    }
  });
  std::thread t3([&]() {
    while (1) {
      m.lock();
      i = (i + 1) % 9;
      m.unlock();
      printf("%c\n", 'a' + i);
    }
  });
  while(1){}
  #endif
  
  VADL2022 vadl(argc, argv);

  std::thread th { [](){
    QueuedFunction f;
    { out_guard();
      std::cout << "Started radio thread" << std::endl; }
    do {
      { out_guard();
        std::cout << "Radio thread waiting for next function..."
                << std::endl; }
      radioDispatchQueue.dequeue(&f);
      { out_guard();
        std::cout << "Radio thread executing function with description: " << f.description << std::endl;  }
      //f.f(); // TODO: implement properly
    } while (mainDispatchQueueRunning);
  }};

  QueuedFunction f;
  mainDispatchQueueRunning = true;
  do {
    { out_guard();
      std::cout << "Main dispatch queue waiting for next function..." << std::endl; }
    mainDispatchQueue.dequeue(&f);
    { out_guard();
      std::cout << "Main dispatch queue executing function with description: " << f.description << std::endl; }
    if (f.type == QueuedFunctionType::Python) {
      isRunningPython = true;
    }
    f.f();
    if (f.type == QueuedFunctionType::Python) {
      isRunningPython = false;
    }
    if (mainDispatchQueueDrainThenStop && mainDispatchQueue.empty()) {
      { out_guard();
        std::cout << "Drained queue, now stopping" << std::endl; }
      mainDispatchQueueRunning = false;
    }
  } while (mainDispatchQueueRunning);

  { out_guard();
    std::cout << "Main dispatch queue no longer running" << std::endl; }

  //std::cout << "Waiting for radio thread to finish..." << std::endl; // TODO: implement properly
  //th.join(); // TODO: implement properly
  { out_guard();
    std::cout << "Radio thread finished" << std::endl; }

  if (videoCaptureOnlyThread) {
    videoCaptureOnlyThread->join();
    videoCaptureOnlyThread.reset(); // Do this here instead of outside main()/in global scope since std threads don't like to be destructor'ed in global scope
  }

  //PyRun_SimpleString("input('Done')");
  return 0;
}

void reportStatus(Status status) {
  // std::string status_str = std::to_string(status);
  // std::string cmd = "python3 subscale_driver/radio.py 0 " + status_str;
  // system(cmd.c_str());

  
  // std::cout << "Reporting status: " << status << std::endl;
  return; // TODO: implement properly
  
  const char* path = "subscale_driver/radio.py";
  radioDispatchQueue.enqueue([=](){
    //PyGILState_STATE state = PyGILState_Ensure(); // Only run this if no other python code is running now, otherwise wait for a lock // TODO: implement properly

    std::string status_str = std::to_string(status);
    const char* args[] = {"0", status_str.c_str(), NULL};
    S_RunFile(path, 2, (char**)args);
    
    //PyGILState_Release(state); // TODO: implement properly
  },path,QueuedFunctionType::Python);
}
