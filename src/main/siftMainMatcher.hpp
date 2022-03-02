//
//  siftMainMatcher.hpp
//  SIFT
//
//  Created by VADL on 3/1/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#ifndef siftMainMatcher_hpp
#define siftMainMatcher_hpp

struct DataSourceBase;
#ifdef USE_COMMAND_LINE_ARGS
extern DataSourceBase* g_src;
#endif

extern void* matcherThreadFunc(void* arg);

#endif /* siftMainMatcher_hpp */
