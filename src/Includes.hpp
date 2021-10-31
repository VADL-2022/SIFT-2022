#ifndef Includes_h
#define Includes_h

#include <stdlib.h>
#include <stdio.h>
#include <lib_sift.h>
#include <lib_sift_anatomy.h>
#include <lib_keypoint.h>
#include <lib_matching.h>
#include <lib_util.h>
#include <io_png.h>

// Use the OpenCV C API ( http://doc.aldebaran.com/2-0/dev/cpp/examples/vision/opencv.html )
//#include <opencv2/core/core_c.h>

#include <opencv2/opencv.hpp>

#include "my_sift_additions.h"

#include <filesystem>
namespace fs = std::filesystem;
//namespace fs = std::__fs::filesystem;
#include "strnatcmp.hpp"

#include "opencv2/highgui.hpp"
#include <optional>
#include <cinttypes>
#include <iostream>
#include "utils.hpp"

//#include <errno.h>
#define ESUCCESS    0        /* Success */ // From https://opensource.apple.com/source/xnu/xnu-344.21.73/osfmk/libsa/errno.h.auto.html

#include "Timer.hpp"

#endif /* Includes_h */
