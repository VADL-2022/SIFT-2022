//
//  strnatcmp.hpp
//  SIFT
//
//  Created by VADL on 10/31/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef strnatcmp_h
#define strnatcmp_h

// https://github.com/Amerge/natsort

#include <cstddef>    /* size_t */
#include <cctype>
#include <string>

typedef char nat_char;

int strnatcmp(nat_char const *a, nat_char const *b);
int strnatcasecmp(nat_char const *a, nat_char const *b);


static inline int nat_isdigit(nat_char a){
     return isdigit((unsigned char) a);
}


static inline int nat_isspace(nat_char a){
     return isspace((unsigned char) a);
}


static inline nat_char nat_toupper(nat_char a){
     return toupper((unsigned char) a);
}


int compare_right(nat_char const *a, nat_char const *b);


int compare_left(nat_char const *a, nat_char const *b);


int strnatcmp0(nat_char const *a, nat_char const *b, int fold_case);

/*The following doesn't consider case of string*/
int strnatcmp(nat_char const *a, nat_char const *b);


/* Compare, recognizing numeric string and ignoring case.
 * This one is the general case where lower-case and upper-
 * case are considered as same.
 *  */
int strnatcasecmp(nat_char const *a, nat_char const *b);

//You can use this function with std::sort and vector
bool compareNat(const std::string& a,const std::string& b);

//You can use this function with qsort
int compareNatq(const void *a, const void *b);

#endif /* strnatcmp_h */
