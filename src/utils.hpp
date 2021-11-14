//
//  utils.hpp
//  SIFT
//
//  Created by VADL on 10/31/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#pragma once
#include <string>

// https://stackoverflow.com/questions/10167534/how-to-find-out-what-type-of-a-mat-object-is-with-mattype-in-opencv
// Usage: mat_type2str(cvMat.type())
std::string mat_type2str(int type);

// Throws if fails
std::string openFileWithUniqueName(std::string name, std::string extension);

// https://stackoverflow.com/questions/874134/find-out-if-string-ends-with-another-string-in-c //
#if __cplusplus >= 201703L // C++17 and later 
#include <string_view>

static bool endsWith(std::string_view str, std::string_view suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

static bool startsWith(std::string_view str, std::string_view prefix)
{
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}
#endif // C++17

#if __cplusplus < 201703L // pre C++17
#include <string>

static bool endsWith(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

static bool startsWith(const std::string& str, const std::string& prefix)
{
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

static bool endsWith(const std::string& str, const char* suffix, unsigned suffixLen)
{
    return str.size() >= suffixLen && 0 == str.compare(str.size()-suffixLen, suffixLen, suffix, suffixLen);
}

static bool endsWith(const std::string& str, const char* suffix)
{
    return endsWith(str, suffix, std::string::traits_type::length(suffix));
}

static bool startsWith(const std::string& str, const char* prefix, unsigned prefixLen)
{
    return str.size() >= prefixLen && 0 == str.compare(0, prefixLen, prefix, prefixLen);
}

static bool startsWith(const std::string& str, const char* prefix)
{
    return startsWith(str, prefix, std::string::traits_type::length(prefix));
}
#endif
// //
