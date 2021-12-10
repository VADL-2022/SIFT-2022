//
//  _putchar.c
//  SIFT
//
//  Created by VADL on 12/9/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "file_descriptors.h"
#include <unistd.h>

// Implemented from printf.h:
/**
 * Output a character to a custom device like UART, used by the printf() function
 * This function is declared here only. You have to write your custom implementation somewhere
 * \param character Character to output
 */
void _putchar(char character) {
    write(STDOUT, &character, 1);
}
