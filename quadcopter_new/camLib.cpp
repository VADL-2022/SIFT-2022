// https://www.manmade2.com/concurrent-c-opencv-video-streaming/

#include "./camLib.hpp" 

using namespace std;

VideoCapture cap(0);		// The openCV camera object...

// The Constructor...
Camera::Camera(void) {
	//Check if opening the camera worked...
	cout << " Camera warming up..." << endl;
	int isrunning = 0;
	usleep(10);
	if (!cap.isOpened())  // if not success, exit program
	{
		cout << "Cannot open the video cam" << endl;
	} else {
		isrunning = 1;
	}

	if(isrunning == 0) {
		cout << "Camara did not start up - Exiting..." << endl;
		this->~Camera();
	}
	
	// Determine camera output size automatically...
	cap.set(CAP_PROP_FRAME_WIDTH, 1280);
	cap.set(CAP_PROP_FRAME_HEIGHT, 720);
	cap.set(CAP_PROP_FPS, 30);
	dWidth  = cap.get(CAP_PROP_FRAME_WIDTH); 		// get the width of frames of the video
    dHeight = cap.get(CAP_PROP_FRAME_HEIGHT); 	// get the height of frames of the video
    fps     = cap.get(CAP_PROP_FPS);				// get frames-per-second of the video device
	// Print values out...
    cout << "Frame size : " << dWidth << " x " << dHeight << " --- fps: " << fps << endl;
    
    // Read in a new frame...
	cap >> frame;		// This frame is furthre processed for the motion detection...	
	//cout << "FIRST Height = " << frame.rows << " .. Width = " << frame.cols << endl;
    
}

// The Destructor...
Camera::~Camera(void)  {
	cout << "Shutting down camera and closing files..." << endl;
	cap.release();	
}
 
//----------------------------------------------------------------------
 
// The camera access function... 
Mat Camera::captureVideo(void) {
	cap >> frame;		// This frame is furthre processed for the motion detection...	
	//cout << "In VideoCapture Height = " << frame.rows << " .. Width = " << frame.cols << endl;
   return frame;
}
