//
//  DataOutput.cpp
//  SIFT
//
//  Created by VADL on 11/3/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "DataOutput.hpp"

#include "opencv2/highgui.hpp"
#include "utils.hpp"

void PreviewWindowDataOutput::showCanvas(std::string name, cv::Mat& canvas) {
    t.reset();
    imshow(name, canvas);
    t.logElapsed("show canvas window");
}

int PreviewWindowDataOutput::waitKey(int delay) {
    return cv::waitKey(delay);
}

FileDataOutput::FileDataOutput(std::string filenameNoExt_, double fps_, cv::Size sizeFrame_) :
fps(fps_), sizeFrame(sizeFrame_), filenameNoExt(filenameNoExt_) {}

void FileDataOutput::run(cv::Mat frame) {
    writerMutex.lock();
    bool first = !writer.isOpened();
    if (first) {
        int codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1');

        bool isColor = true; //(frame.channels() > 1);
        //std::cout << "Output type: " << mat_type2str(frame.type()) << "\nisColor: " << isColor << std::endl;
        std::string filename = openFileWithUniqueName(filenameNoExt, ".mp4");
        writer.open(filename, codec, fps, sizeFrame, isColor);
    }
    writerMutex.unlock();

    // Resize/convert, then write to video writer
    cv::Mat newFrame;
    cv::Mat* newFramePtr;
    std::cout << "frame.data: " << (void*)frame.data << std::endl;
    if (frame.cols != sizeFrame.width || frame.rows != sizeFrame.height) {
        t.reset();
        cv::resize(frame, newFrame, sizeFrame);
        newFramePtr = &newFrame;
        t.logElapsed("resize frame for video writer");
    }
    else {
        newFramePtr = &frame;
    }
    if (newFramePtr->type() != CV_8UC4 || newFramePtr->type() != CV_8UC3) {
        t.reset();
        newFramePtr->convertTo(newFrame, CV_8U);//, 255); // https://stackoverflow.com/questions/22174002/why-does-opencvs-convertto-function-not-work : need to scale the values up to char's range of 0-255
        newFramePtr = &newFrame;
        t.logElapsed("convert frame for video writer part 1");
    }
    if (newFramePtr->type() == CV_8UC4) {
        t.reset();
        std::cout << "Converting from " << mat_type2str(newFramePtr->type()) << std::endl;
        // https://answers.opencv.org/question/66545/problems-with-the-video-writer-in-opencv-300/
        cv::cvtColor(*newFramePtr, newFrame, cv::COLOR_RGBA2BGR);
        newFramePtr = &newFrame;
        t.logElapsed("convert frame for video writer part 2");
    }
    t.reset();
    std::cout << "newFrame.data: " << (void*)newFrame.data << std::endl;
    std::cout << "Writing frame with type " << mat_type2str(newFrame.type()) << std::endl;
    writerMutex.lock();
    writer.write(newFrame);
    writerMutex.unlock();
    t.logElapsed("write frame for video writer");
}

void FileDataOutput::release() {
    writerMutex.lock();
    if (!releasedWriter) {
        writer.release(); // Save the file
        std::cout << "Saved the video" << std::endl;
        releasedWriter = true;
        writerMutex.unlock();
    }
    else {
        writerMutex.unlock();
        std::cout << "Video was saved already" << std::endl;
    }
}

void FileDataOutput::showCanvas(std::string name, cv::Mat& canvas) {
    t.reset();
    run(canvas);
    t.logElapsed("save frame to FileDataOutput");
}

int FileDataOutput::waitKey(int delay) {
    // Advance to next image after user presses enter
    std::cout << "Press enter to advance to the next frame and get keypoints, g to load or make cached keypoints file, s to apply previous transformations, or q to quit: " << std::flush;
//        std::string n;
//        std::getline(std::cin, n);
    char n = getchar();
    
    if (n == '\n') {
        return 'd'; // Simply advance to next image
    }
    else {
        return n; // The command given
    }
}
