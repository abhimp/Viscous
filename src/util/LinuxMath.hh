/*
 * This is an implemetation of Viscous protocol.
 * Copyright (C) 2017  Abhijit Mondal
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/*
 * LinuxMath.hh
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
