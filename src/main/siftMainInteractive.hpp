//
//  siftMainInteractive.hpp
//  SIFT
//
//  Created by VADL on 3/1/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#ifndef siftMainInteractive_hpp
#define siftMainInteractive_hpp

#include "../Config.hpp"

#ifdef USE_COMMAND_LINE_ARGS
#ifdef SIFTAnatomy_
struct DataSourceBase;
struct SIFTState;
struct SIFTParams;
struct DataOutputBase;
struct FileDataOutput;

extern
int mainInteractive(DataSourceBase* src,
                    SIFTState& s,
                    SIFTParams& p,
                    size_t skip,
                    DataOutputBase& o
                    #ifdef USE_COMMAND_LINE_ARGS
                      , FileDataOutput& o2,
                      const CommandLineConfig& cfg
                    #endif
);
#endif
#endif

#endif /* siftMainInteractive_hpp */
