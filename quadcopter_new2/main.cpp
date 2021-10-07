//! \example tutorial-grabber-opencv-threaded.cpp
//! [capture-multi-threaded declaration]
#include <iostream>

// #include <visp3/core/vpImageConvert.h>
// #include <visp3/core/vpMutex.h>
// #include <visp3/core/vpThread.h>
// #include <visp3/core/vpTime.h>
// #include <visp3/gui/vpDisplayGDI.h>
// #include <visp3/gui/vpDisplayX.h>

//#if (VISP_HAVE_OPENCV_VERSION >= 0x020100) && (defined(VISP_HAVE_PTHREAD) || defined(_WIN32))

#include <opencv2/highgui/highgui.hpp>
#include <mutex>
#include <chrono>
#include <thread>
#include <atomic>
#include "../src/utils.cpp"

// https://stackoverflow.com/questions/1641182/how-can-i-catch-a-ctrl-c-event
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fstream>


std::mutex stdoutMutex;
#define LOG_THREAD_INFO 0
#if LOG_THREAD_INFO
#define logNonMainThreadInfo(x) stdoutMutex.lock(); std::cout << "[thread] " << x << std::flush; stdoutMutex.unlock();
#define logMainThreadInfo(x) stdoutMutex.lock(); std::cout << "[main] " << x << std::flush; stdoutMutex.unlock();
#define logThreadInfo(x) stdoutMutex.lock(); std::cout << x << std::flush; stdoutMutex.unlock();
#else
#define logNonMainThreadInfo(x)
#define logMainThreadInfo(x)
#define logThreadInfo(x)
#endif
#define logNonMainThreadGeneral(x) stdoutMutex.lock(); std::cout << "[thread] " << x << std::flush; stdoutMutex.unlock();
#define logNonMainThreadWarning(x) stdoutMutex.lock(); std::cout << "[thread] [warning] " << x << std::flush; stdoutMutex.unlock();
#define logNonMainThreadError(x) stdoutMutex.lock(); std::cout << "[thread] [error] " << x << std::flush; stdoutMutex.unlock();
#define logMainThreadGeneral(x) stdoutMutex.lock(); std::cout << "[main] " << x << std::flush; stdoutMutex.unlock();
#define logMainThreadWarning(x) stdoutMutex.lock(); std::cout << "[main] [warning] " << x << std::flush; stdoutMutex.unlock();
#define logMainThreadError(x) stdoutMutex.lock(); std::cout << "[main] [error] " << x << std::flush; stdoutMutex.unlock();

#define logInfo(x) stdoutMutex.lock(); std::cout << x << std::flush; stdoutMutex.unlock();
#define logWarning(x) stdoutMutex.lock(); std::cout << "[warning] " << x << std::flush; stdoutMutex.unlock();


std::atomic<bool> ctrlC = false;
void my_handler(int s){
           printf("Caught signal %d\n",s);
	   
           ctrlC = true;

           //exit(0);
}

// Shared vars
typedef enum { capture_waiting, capture_started, capture_stopped } t_CaptureState;
t_CaptureState s_capture_state = capture_waiting;
//cv::Mat s_frame;
std::mutex s_mutex_capture; //vpMutex s_mutex_capture;
//! [capture-multi-threaded declaration]

struct vpThread {
  typedef void* Return;
  typedef void* Args;
  typedef Return (*Fn)(Args);
};

struct vpTime {
  static double measureTimeSecond() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
  }
};

struct vpMutex {
  typedef std::lock_guard<std::mutex> vpScopedLock;
};

double fps;
cv::Size sizeFrame(640,480);
//cv::Size sizeFrame(1920,1080);
std::string filename = "live2.mp4";

template<typename T> class Queue {
public:
  Queue() {
    reserve(32);
  }
  
  void reserve(size_t count) {
    m_vec.resize(count);
    logInfo("m_vec data: " << m_vec.data() << std::endl);
  }

  template< class... Args >
  void push( Args&&... args ) {
    //std::lock_guard<std::mutex> guard(m_mtx);
    assert(!full());
    m_vec[writeIndex++] = T(args...);
    writeIndex = writeIndex % m_vec.size();
    //std::lock_guard<std::mutex> guard(m_mtx);
    m_empty = false;
    
    // if (readIndex == writeIndex) {
    //   // Grow
    //   m_vec.resize(m_vec.capacity() * 2);
    //   writeIndex = m_vec.size(); // Wrong, fix
    // }
  }
  
  T& back() {
    // writeIndex - 1 is because writeIndex always points to the next empty element.
    size_t writtenIndex = (writeIndex - 1) % m_vec.size();
    //std::lock_guard<std::mutex> guard(m_mtx); // Optional
    logInfo("back: " << readIndex << " " << writeIndex << " " << &m_vec[writtenIndex] << std::endl);
    return m_vec[writtenIndex];
  }

  T& front() {
    //std::lock_guard<std::mutex> guard(m_mtx); // Optional
    logInfo("front: " << readIndex << " " << writeIndex << " " << &m_vec[readIndex] << std::endl);
    return m_vec[readIndex];
  }

  void pop() {
    //std::lock_guard<std::mutex> guard(m_mtx);
    assert(!empty());
    readIndex++;
    readIndex = readIndex % m_vec.size();
    // Actual lock: std::lock_guard<std::mutex> guard(m_mtx);
    if (readIndex == writeIndex) {
      m_empty = true;
    }
  }

  // Not thread safe
  bool empty() {
    return readIndex == writeIndex && m_empty;
  }

  // Not thread safe
  bool full() {
    return readIndex == writeIndex && !m_empty;
  }

  void lock() {
    m_mtx.lock();
  }
  
  void unlock() {
    m_mtx.unlock();
  }
private:
  std::mutex m_mtx;
  std::vector<T> m_vec;
  size_t writeIndex = 0;
  size_t readIndex = 0;
  bool m_empty = true;
};

Queue<cv::Mat> imageQueue;
enum LogEntryType {
  Time,
  EverythingExceptCVTime
};
struct LogEntry {
  static std::chrono::time_point<std::chrono::steady_clock> start;
  static bool first;
  static std::mutex startMutex;
  LogEntry(LogEntryType type, cv::VideoCapture* cap) {
      auto now = std::chrono::steady_clock::now();
      startMutex.lock();
      if (first) {
	start = now;
	first = false;
      }
      startMutex.unlock();
      std::chrono::nanoseconds::rep elapsedTotal = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start).count();

      // Save stuff
      this->type = type;
      if (cap != nullptr)
	this->cvTime = cap->get(cv::CAP_PROP_POS_MSEC);
      this->steadyClockTime = elapsedTotal;

      if (type != LogEntryType::Time) {
	// Save temperature //
	// https://www.raspberrypi.org/forums/viewtopic.php?t=252115
	FILE *fp;
	int temp = INT_MIN;
	fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
	fscanf(fp, "%d", &temp);
	auto celcius = temp / 1000.0;
	//printf(">> CPU Temp: %.2f°C\n", celcius);
	fclose(fp);
	// //
	// Save throttle status //
	fp;
	char chars[256];
	int throttleStatus = INT_MIN; // Documentation for this: https://github.com/raspberrypi/firmware/blob/abc347435437f6e2e85b5f367e75a5114882eaa3/hardfp/opt/vc/man/man1/vcgencmd.1
      
	/* Open the command for reading. */
	fp = popen("/usr/bin/vcgencmd get_throttled", "r"); //popen("/usr/bin/vcgencmd measure_temp", "r");
	//fp = popen("echo throttled=0x23", "r");
	if (fp == NULL) {
	  printf("Failed to run command\n" );
	  exit(1);
	}

	/* Read the output a line at a time - output it. */
	//while (fgets(chars, sizeof(chars), fp) != NULL) {
	  //printf("%s", path);
	  if (fscanf(fp, "throttled=%i", &throttleStatus) <= 0) {
	    perror("Failed to read in throttle status in fscanf");
	    exit(1);
	  }
	  // break;
	// }

	/* close */
	pclose(fp);
	// //
	// Save clock speed //
	int clockSpeed = INT_MIN; // Hz

	/* Open the command for reading. */
	fp = popen("/usr/bin/vcgencmd measure_clock arm", "r");
	if (fp == NULL) {
	  printf("Failed to run command\n" );
	  exit(1);
	}

	/* Read the output a line at a time - output it. */
	//while (fgets(chars, sizeof(chars), fp) != NULL) {
	  //printf("%s", path);
	int retval = fscanf(fp, "frequency(48)=%d", &clockSpeed);
	//std::cout << "Returned " << retval << std::endl;
	  if (retval <= 0) {
	    perror("Failed to read in frequency in fscanf");
	    exit(1);
	  }
	  // break;
	// }

	/* close */
	pclose(fp);
	// //

	// Save stuff
	this->celcius = temp;
	this->throttleStatus = throttleStatus;
	this->clockSpeed = clockSpeed;
      }
  }
  LogEntryType type;
  std::chrono::nanoseconds::rep steadyClockTime;
  double cvTime;
  int celcius;
  int throttleStatus;
  int clockSpeed;
};
std::chrono::time_point<std::chrono::steady_clock> LogEntry::start;
bool LogEntry::first = true;
std::mutex LogEntry::startMutex;

std::vector<LogEntry> logs; // Logged timestamps, etc.
std::mutex logsMutex;

#define startTimer() std::chrono::high_resolution_clock::now()
#define stopTimer(startTime) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count()
//! [capture-multi-threaded captureFunction]
vpThread::Return captureFunction(vpThread::Args args)
{
  cv::VideoCapture cap = *((cv::VideoCapture *)args);

  if (!cap.isOpened()) { // check if we succeeded
    std::cout << "Unable to start capture" << std::endl;
    return 0;
  }
  

  // https://learnopencv.com/how-to-find-frame-rate-or-frames-per-second-fps-in-opencv-python-cpp/
  // "For OpenCV 3, you can also use the following"
  fps = cap.get(cv::CAP_PROP_FPS);
  
  cap.set(cv::CAP_PROP_FRAME_WIDTH, sizeFrame.width);
  cap.set(cv::CAP_PROP_FRAME_HEIGHT, sizeFrame.height);
  double width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
  double height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
  std::cout << "Frames per second using video.get(CAP_PROP_FPS) : " << fps << "\nWidth: " << width << "\nHeight: " << height << std::endl;

  
  //cv::Mat frame_;
  // int i = 0;
  // while ((i++ < 100) && !cap.read(frame_)) {
  // }; // warm up camera by skiping unread frames

  bool stop_capture_ = false;

  //double start_time = vpTime::measureTimeSecond();
  while (//(vpTime::measureTimeSecond() - start_time) < 30 &&
	 !stop_capture_) {
    // Capture in progress
    auto start = startTimer();
    imageQueue.lock();
    if (!imageQueue.full()) {
      imageQueue.push();
      cv::Mat& frame_ = imageQueue.back();
      cap >> frame_; // get a new frame from camera
      // Save timestamp //
      LogEntry o{LogEntryType::Time, &cap};
      logsMutex.lock();
      logs.push_back(o); // Save timestamp and other stuff
      logsMutex.unlock();
      // //
      imageQueue.unlock();
    }
    else {
      imageQueue.unlock();
      logNonMainThreadWarning("Image queue full" << std::endl);
    }

    // Update shared data
    {
      vpMutex::vpScopedLock lock(s_mutex_capture);
      if (s_capture_state == capture_stopped)
        stop_capture_ = true;
      else
        s_capture_state = capture_started;
      //s_frame = frame_;
    }
    //std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    auto duration = stopTimer(start);
    logNonMainThreadGeneral(duration << " milliseconds" << std::endl);
  }

  {
    vpMutex::vpScopedLock lock(s_mutex_capture);
    s_capture_state = capture_stopped;
  }

  std::cout << "End of capture thread" << std::endl;
  return 0;
}
//! [capture-multi-threaded captureFunction]

cv::VideoWriter writer;

//! [capture-multi-threaded displayFunction]
vpThread::Return displayFunction(vpThread::Args args)
{
  (void)args; // Avoid warning: unused parameter args
  //vpImage<unsigned char> I_;

  t_CaptureState capture_state_;
  bool display_initialized_ = false;
#if defined(VISP_HAVE_X11)
  vpDisplayX *d_ = NULL;
#elif defined(VISP_HAVE_GDI)
  vpDisplayGDI *d_ = NULL;
#endif

   int codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
   bool first = true;
  do {
    auto start = startTimer();
    s_mutex_capture.lock();
    capture_state_ = s_capture_state;
    s_mutex_capture.unlock();

    // Check if a frame is available
    if (capture_state_ == capture_started) {
      // Get the frame and convert it to a ViSP image used by the display
      // class
      // {
      //   vpMutex::vpScopedLock lock(s_mutex_capture);
      //   //vpImageConvert::convert(s_frame, I_);
      // }

      // Check if we need to initialize the display with the first frame
      if (!display_initialized_) {
// Initialize the display
#if defined(VISP_HAVE_X11)
        d_ = new vpDisplayX(I_);
        display_initialized_ = true;
#elif defined(VISP_HAVE_GDI)
        d_ = new vpDisplayGDI(I_);
        display_initialized_ = true;
#endif
      }

      // // Display the image
      // vpDisplay::display(I_);
      //std::cout << "imshow" << std::endl;
      // imshow("s_frame", s_frame);
      // cv::pollKey();

      //logMainThreadWarning("hi");
      imageQueue.lock();
      if (!imageQueue.empty()) {
	cv::Mat& s_frame = imageQueue.front();
	imageQueue.unlock();
	assert(!s_frame.empty());
	
	if (first) {
	  first = false;
	
	  bool isColor = (s_frame.type() == CV_8UC3);
	  std::cout << "Output type: " << type2str(s_frame.type()) << "\nisColor: " << isColor << std::endl;
	  writer.open(filename, codec, fps, sizeFrame, isColor);
	}
	
	writer.write(s_frame);
	imageQueue.lock();
	imageQueue.pop();
	imageQueue.unlock();
      }
      else {
	imageQueue.unlock();
	//logMainThreadWarning("Image queue empty" << std::endl);
	std::this_thread::sleep_for(std::chrono::milliseconds(20));
      }

      // // Trigger end of acquisition with a mouse click
      // vpDisplay::displayText(I_, 10, 10, "Click to exit...", vpColor::red);
      // if (vpDisplay::getClick(I_, false)) {
      //   vpMutex::vpScopedLock lock(s_mutex_capture);
      //   s_capture_state = capture_stopped;
      // }

      // // Update the display
      // vpDisplay::flush(I_);
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
      //vpTime::wait(2); // Sleep 2ms
    }
    //std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    auto duration = stopTimer(start);
    //logMainThreadGeneral(duration << " milliseconds" << std::endl);
  } while (capture_state_ != capture_stopped && !ctrlC);

#if defined(VISP_HAVE_X11) || defined(VISP_HAVE_GDI)
  delete d_;
#endif

  std::cout << "End of display thread" << std::endl;
  return 0;
}
//! [capture-multi-threaded displayFunction]

#define RunOptionsStruct				\
struct RunOptions {					\
  union {						\
    struct {						\
	unsigned long enableCaptureThread: 1;		\
	unsigned long enableVideoSavingThread: 1;	\
	unsigned long enableLoggerThread: 1;		\
    };							\
    unsigned long integerValue;				\
  };							\
};
#define str(s) #s
RunOptionsStruct
//! [capture-multi-threaded mainFunction]
int main(int argc, const char *argv[])
{
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = my_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);
  
  int opt_device = 0;

  // Command line options
  RunOptions runOptions = {1, 1, 1};
  std::cout << runOptions.integerValue << std::endl;
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "--camera_device")
      opt_device = atoi(argv[i + 1]);
    else if (std::string(argv[i]) == "--run_options") {
      // Implementation-specific thing is causing the bits to need to be reversed, else the bitset is set in reversed order.
      #ifdef __aarch64__
      std::string s = argv[i + 1];
      std::reverse(s.begin(), s.end());
#elif defined(__amd64___)
      std::string s = argv[i + 1];
      #else
      #error "Not yet implemented platform"
      #endif
      
      unsigned long result = std::stoul(s.c_str(), nullptr, 2);
      runOptions.integerValue = result;
      i++;
    }
    else { //if (std::string(argv[i]) == "--help") {
      std::cout << argv[i] << std::endl;
      std::cout << "Usage: " << argv[0] << " [--camera_device <camera device (default: 0)>] [--run_options <bitset like 010: " str(RunOptionsStruct) "\n>] [--help]" << std::endl;
      return 1;
    }
  }
  std::cout << runOptions.integerValue << std::endl;
  // std::cout << runOptions.enableCaptureThread << std::endl;
  // std::cout << runOptions.enableVideoSavingThread << std::endl;
  // std::cout << runOptions.enableLoggerThread << std::endl;

  // Instanciate the capture
  cv::VideoCapture cap;
  if (runOptions.enableCaptureThread) {
    cap.open(opt_device);
  }

  // Start the threads
  std::thread thread_capture;
  if (runOptions.enableCaptureThread) {
    thread_capture = std::thread((vpThread::Fn)captureFunction, (vpThread::Args)&cap);
  }
  //std::thread thread_display((vpThread::Fn)displayFunction, (vpThread::Args)nullptr);
  std::thread thread_logger;
  if (runOptions.enableLoggerThread) {
    thread_logger = std::thread([](){
      while (!ctrlC) {
	//logNonMainThreadGeneral("log\n");
	LogEntry o{LogEntryType::EverythingExceptCVTime, nullptr};
	logsMutex.lock();
	logs.push_back(o); // Save timestamp and other stuff
	logsMutex.unlock();
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    });
  }
  if (runOptions.enableVideoSavingThread) {
    displayFunction(nullptr);
  }

  // Wait until thread ends up
  if (runOptions.enableCaptureThread) {
    thread_capture.join();
  }
  if (runOptions.enableLoggerThread) {
    thread_logger.join();
  }
  //thread_display.join();

  if (ctrlC) {
    std::cout << ("[ctrl-c handler] Got the signal\n"); // response message

    s_mutex_capture.lock();
    s_capture_state = capture_stopped;
    s_mutex_capture.unlock();
      
    // Save video
    writer.release();

    // Save times
    std::ofstream FILE("timestamps.csv", std::ios::out | std::ofstream::binary | std::ios_base::trunc);
    FILE << "steadyClockTime,cvTime,celcius,throttleStatus,clockSpeed" << std::endl;
    for (auto log : logs) {
      switch (log.type) {
      case Time:
	FILE << log.steadyClockTime << "," << log.cvTime << ",,," << std::endl;
	break;
      case EverythingExceptCVTime:
	FILE << log.steadyClockTime << ",," << log.celcius << "," << log.throttleStatus << "," << log.clockSpeed << std::endl;
	break;
      }
    }
    FILE.close();

    //break;
  }

  return 0;
}
//! [capture-multi-threaded mainFunction]

// #else
// int main()
// {
// #ifndef VISP_HAVE_OPENCV
//   std::cout << "You should install OpenCV to make this example working..." << std::endl;
// #elif !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))) // UNIX
//   std::cout << "You should enable pthread usage and rebuild ViSP..." << std::endl;
// #else
//   std::cout << "Multi-threading seems not supported on this platform" << std::endl;
// #endif
// }

// #endif
