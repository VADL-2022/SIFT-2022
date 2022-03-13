//
//  mat_wrapper.cpp
//  SIFT
//
//  Created by VADL on 3/12/22.
//  Copyright © 2022 VADL. All rights reserved.
//
// Based on https://www.twblogs.net/a/5d400d6cbd9eee517422e088?lang=zh-cn

#include "mat_wrapper.hpp"

#include <pybind11/numpy.h>
//#include<opencv2/opencv.hpp>
/*
Python->C++ Mat
*/

//namespace py = pybind11;

cv::Mat numpy_uint8_1c_to_cv_mat(py::array_t<unsigned char>& input)
{

    if (input.ndim() != 2)
        throw std::runtime_error("1-channel image must be 2 dims ");

    py::buffer_info buf = input.request();

    cv::Mat mat(buf.shape[0], buf.shape[1], CV_8UC1, (unsigned char*)buf.ptr);

    return mat;
}


cv::Mat numpy_uint8_3c_to_cv_mat(py::array_t<unsigned char>& input) {

    if (input.ndim() != 3)
        throw std::runtime_error("3-channel image must be 3 dims ");

    py::buffer_info buf = input.request();

    cv::Mat mat(buf.shape[0], buf.shape[1], CV_8UC3, (unsigned char*)buf.ptr);

    return mat;
}

cv::Mat numpy_float32_1c_to_cv_mat(py::array_t<float>& input) {
    
    if (input.ndim() != 2)
        throw std::runtime_error("1-channel image must be 2 dims ");

    py::buffer_info buf = input.request();

    cv::Mat mat(buf.shape[0], buf.shape[1], CV_32FC1, (unsigned char*)buf.ptr);

    return mat;
}

/*
C++ Mat ->numpy
*/
py::array_t<unsigned char> cv_mat_uint8_1c_to_numpy(cv::Mat& input) {

    py::array_t<unsigned char> dst = py::array_t<unsigned char>({ input.rows,input.cols }, input.data);
    return dst;
}

py::array_t<unsigned char> cv_mat_uint8_3c_to_numpy(cv::Mat& input) {

    py::array_t<unsigned char> dst = py::array_t<unsigned char>({ input.rows,input.cols,3 }, input.data);
    return dst;
}

py::array_t<float> cv_mat_float32_1c_to_numpy(cv::Mat& input) {
    
    py::array_t<float> dst = py::array_t<float>({ input.rows,input.cols }, (float*)input.data);
    return dst;
}



//PYBIND11_MODULE(cv_mat_warper, m) {
//
//  m.doc() = "OpenCV Mat -> Numpy.ndarray warper";
//
//  m.def("numpy_uint8_1c_to_cv_mat", &numpy_uint8_1c_to_cv_mat);
//  m.def("numpy_uint8_1c_to_cv_mat", &numpy_uint8_1c_to_cv_mat);
//
//
//}
