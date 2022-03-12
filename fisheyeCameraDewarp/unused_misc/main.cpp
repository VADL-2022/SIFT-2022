// https://gist.github.com/suzumura-ss/db91b901f5a300e6b9949cf5e012278e

#include <iostream>
#include <opencv2/opencv.hpp>



int main(int argc, char* argv[])
{
    // based on https://medium.com/@kennethjiang/calibrate-fisheye-lens-using-opencv-333b05afa0b0

    cv::CommandLineParser parser(argc, argv,
        "{h||help}"
        "{W|6|checkerboard width}"
        "{H|9|checkerboard height}"
        "{@source||source images}"
    );
    if (parser.has("h")) {
        parser.printMessage();
        return 1;
    }

    const cv::Size checkerBoard(parser.get<int>("W"), parser.get<int>("H"));
    std::vector<cv::Point3f> objp;
    cv::Size graySize;
    std::vector<std::vector<cv::Point3f>> objpoints;
    std::vector<std::vector<cv::Point2f>> imgpoints;
    std::vector<std::string> sources;

    for (int v = 0; v < checkerBoard.height; ++v) {
        for (int u = 0; u < checkerBoard.width;  ++u) {
            objp.push_back(cv::Point3f(float(u), float(v), 0));
        }
    }

    for (int i = 1; i < argc; ++i) {
        std::string fileName(argv[i]);
        if (fileName.begin()[0] == '-') continue;
        std::cout << "Processing " << fileName << "..." << std::endl;

        cv::Mat source = cv::imread(fileName);
        if (source.empty()) {
            std::cout << "Failed to imread(" << fileName << ")." << std::endl;
            continue;
        }

        cv::Mat gray;
        cv::cvtColor(source, gray, cv::COLOR_RGB2GRAY);
        if (graySize.area() == 0) {
            graySize = gray.size();
        } else if (gray.size() != graySize) {
            std::cout << "Invalid image size: " << fileName << " - expect " << graySize << ", but " << gray.size() << std::endl;
            continue;
        }
        sources.push_back(fileName);

        std::vector<cv::Point2f> corners;
        int flags = cv::CALIB_CB_ADAPTIVE_THRESH|cv::CALIB_CB_FAST_CHECK|cv::CALIB_CB_NORMALIZE_IMAGE;
        if (!cv::findChessboardCorners(gray, checkerBoard, corners, flags)) {
            std::cout << "Failed to findChessboardCorners(" << fileName << ")." << std::endl;
            continue;
        }
        objpoints.push_back(objp);
        cv::TermCriteria criteria(cv::TermCriteria::EPS|cv::TermCriteria::MAX_ITER, 30, 0.1);
        cv::cornerSubPix(gray, corners, cv::Size(3, 3), cv::Size(-1, -1), criteria);
        imgpoints.push_back(corners);
    }

    std::cout << "Processing cv::fisheye::calibrate() ..." << std::endl;
    cv::Mat K, D;
    std::vector<cv::Mat> rvecs, tvecs;
    int flags = cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC|cv::fisheye::CALIB_FIX_SKEW;
    cv::TermCriteria criteria(cv::TermCriteria::EPS|cv::TermCriteria::MAX_ITER, 30, 1e-6);
    cv::fisheye::calibrate(objpoints, imgpoints, graySize, K, D, rvecs, tvecs, flags, criteria);

    std::cout
        << "K=" << K << std::endl
        << "D=" << D << std::endl;

    cv::Mat map1, map2;
    cv::fisheye::initUndistortRectifyMap(K, D, cv::Mat::eye(3, 3, CV_32F), K, graySize, CV_16SC2, map1, map2);

    for (auto fileName : sources) {
        cv::Mat source = cv::imread(fileName);
        cv::Mat undistort;
        cv::remap(source, undistort, map1, map2, cv::INTER_LINEAR, cv::BORDER_CONSTANT);

        cv::Mat result;
        cv::hconcat(source, undistort, result);
        cv::resize(result, result, cv::Size(), 0.25, 0.25);
        cv::imshow("result", result);
        cv::waitKey();
    }

    return 0;
}
