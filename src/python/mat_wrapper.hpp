//
//  mat_wrapper.hpp
//  SIFT
//
//  Created by VADL on 3/12/22.
//  Copyright © 2022 VADL. All rights reserved.
//
// Based on https://www.twblogs.net/a/5d400d6cbd9eee517422e088?lang=zh-cn

#ifndef mat_wrapper_hpp
#define mat_wrapper_hpp

#include<opencv2/opencv.hpp>
#include<pybind11/pybind11.h>
#include<pybind11/numpy.h>

namespace py = pybind11;

cv::Mat numpy_uint8_1c_to_cv_mat(py::array_t<unsigned char>& input);

cv::Mat numpy_uint8_3c_to_cv_mat(py::array_t<unsigned char>& input);

cv::Mat numpy_float32_1c_to_cv_mat(py::array_t<float>& input);

py::array_t<unsigned char> cv_mat_uint8_1c_to_numpy(cv::Mat& input);

py::array_t<unsigned char> cv_mat_uint8_3c_to_numpy(cv::Mat& input);

py::array_t<float> cv_mat_float32_1c_to_numpy(cv::Mat& input);

#endif /* mat_wrapper_hpp */
