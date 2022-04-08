// https://stackoverflow.com/questions/34316306/opencv-fisheye-calibration-cuts-too-much-of-the-resulting-image

#include <iostream>
#include <sstream>
#include <time.h>
#include <stdio.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>

// - compile with:
// g++ -ggdb `pkg-config --cflags --libs opencv` fist2rect.cpp -o fist2rect
// - execute:
// fist2rect input.jpg output.jpg

 using namespace std;
 using namespace cv;
 #define PI 3.1415926536

 Point2f getInputPoint(int x, int y,int srcwidth, int srcheight)
 {
    Point2f pfish;
    float theta,phi,r, r2;
    Point3f psph;
    float FOV =(float)PI/180 * 180;
    float FOV2 = (float)PI/180 * 180;
    float width = srcwidth;
    float height = srcheight;

    // Polar angles
    theta = PI * (x / width - 0.5); // -pi/2 to pi/2
    phi = PI * (y / height - 0.5);  // -pi/2 to pi/2

    // Vector in 3D space
    psph.x = cos(phi) * sin(theta);
    psph.y = cos(phi) * cos(theta);
    psph.z = sin(phi) * cos(theta);

    // Calculate fisheye angle and radius
    theta = atan2(psph.z,psph.x);
    phi = atan2(sqrt(psph.x*psph.x+psph.z*psph.z),psph.y);

    r = width * phi / FOV;
    r2 = height * phi / FOV2;

    // Pixel in fisheye space
    pfish.x = 0.5 * width + r * cos(theta);
    pfish.y = 0.5 * height + r2 * sin(theta);
    return pfish;
}
int main(int argc, char **argv)
{
    if(argc< 3)
        return 0;
    Mat orignalImage = imread(argv[1]);
    if(orignalImage.empty())
    {
        cout<<"Empty image\n";
        return 0;
    }
    Mat outImage(orignalImage.rows,orignalImage.cols,CV_8UC3);

    namedWindow("result",cv::WINDOW_NORMAL);

    for(int i=0; i<outImage.cols; i++)
    {
        for(int j=0; j<outImage.rows; j++)
        {

            Point2f inP =  getInputPoint(i,j,orignalImage.cols,orignalImage.rows);
            Point inP2((int)inP.x,(int)inP.y);

            if(inP2.x >= orignalImage.cols || inP2.y >= orignalImage.rows)
                continue;

            if(inP2.x < 0 || inP2.y < 0)
                continue;
            Vec3b color = orignalImage.at<Vec3b>(inP2);
            outImage.at<Vec3b>(Point(i,j)) = color;

        }
    }

    imwrite(argv[2],outImage);

}
