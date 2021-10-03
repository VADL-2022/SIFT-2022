#pragma once
#include <string>

// https://stackoverflow.com/questions/10167534/how-to-find-out-what-type-of-a-mat-object-is-with-mattype-in-opencv
// Usage: type2str(cvMat.type())
std::string type2str(int type);
