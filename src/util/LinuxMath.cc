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
 * linux-math.cc
 *
 *  Created on: 25-Dec-2016
 *      Author: abhijit
 */
#include "LinuxMath.hh"



int fls(int x)
{
        int r = 32;

        if (!x)
                return 0;
        if (!(x & 0xffff0000u)) {
                x <<= 16;
                r -= 16;
        }
        if (!(x & 0xff000000u)) {
                x <<= 8;
                r -= 8;
        }
        if (!(x & 0xf0000000u)) {
                x <<= 4;
                r -= 4;
        }
        if (!(x & 0xc0000000u)) {
                x <<= 2;
                r -= 2;
        }
        if (!(x & 0x80000000u)) {
                x <<= 1;
                r -= 1;
        }
        return r;
}

int fls64(appInt64 x)
{
        appInt32 h = x >> 32;
        if (h)
                return fls(h) + 32;
        return fls(x);
}

appInt64 div64_64(appInt64 dividend, appInt64 divisor)
{
    appInt32 high, d;

    high = divisor >> 32;
    if (high) {
        unsigned int shift = fls(high);

        d = divisor >> shift;
        dividend >>= shift;
    } else
        d = divisor;

    do_div(dividend, d);

    return dividend;
}
