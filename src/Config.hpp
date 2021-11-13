//
//  Config.hpp
//  SIFT
//
//  Created by VADL on 11/10/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef Config_h
#define Config_h

// Config //
//#define USE_COMMAND_LINE_ARGS
// //

#ifdef USE_COMMAND_LINE_ARGS
struct CommandLineConfig {
    bool imageCaptureOnly = false, imageFileOutput = false, folderDataSource = false;
};
#endif

#endif /* Config_h */
