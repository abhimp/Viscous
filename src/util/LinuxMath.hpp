/*
 * LinuxMath.hpp
 *
 *  Created on: 25-Dec-2016
 *      Author: abhijit
 */

#ifndef SRC_UTIL_LINUXMATH_HPP_
#define SRC_UTIL_LINUXMATH_HPP_
#include <common.h>
///// For 64 bit division from include/asm-generic/div64.h ////
#define do_div(n,base) ({                                      \
        appInt32 __base = (base);                               \
        appInt32 __rem;                                         \
        __rem = ((appInt64)(n)) % __base;                       \
        (n) = ((appInt64)(n)) / __base;                         \
        __rem;                                                  \
 })

appInt64 div64_64(appInt64 dividend, appInt64 divisor);

int fls64(appInt64 x);

#endif /* SRC_UTIL_LINUXMATH_HPP_ */
