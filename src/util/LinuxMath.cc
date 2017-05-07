/*
 * linux-math.cc
 *
 *  Created on: 25-Dec-2016
 *      Author: abhijit
 */
#include "LinuxMath.hpp"



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
