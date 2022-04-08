//
//  siftMainCmdLineParser.hpp
//  SIFT
//
//  Created by VADL on 3/1/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#ifndef siftMainCmdLineParser_hpp
#define siftMainCmdLineParser_hpp

#include <stdio.h>
#include <memory>

extern int driverInput_fd; // File descriptor for reading from the driver program, if any (else it is -1)
extern int driverInput_fd_fcntl_flags;
extern FILE* driverInput_file;
extern int imageStream_fd; // File descriptor for reading images from shared memory, if any (else it is -1)

struct SIFTState;
struct SIFTParams;
struct DataSourceBase;
extern int parseCommandLineArgs(int argc, char** argv,
                                //Outputs:
                                SIFTState& s, SIFTParams& p, size_t& skip, std::unique_ptr<DataSourceBase>& src);

#endif /* siftMainCmdLineParser_hpp */
