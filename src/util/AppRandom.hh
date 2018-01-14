/*
 * AppRandom.h
 *
 *  Created on: 23-Oct-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_APPRANDOM_HH_
#define SRC_UTIL_APPRANDOM_HH_

namespace util {
static unsigned long int next = 1;

inline int appRand(void) // RAND_MAX assumed to be 32767
{
    next = next * 1103515245 + 12345;
    return (unsigned int)(next/65536) % 32768;
}

inline void appSrand(unsigned int seed)
{
    next = seed;
}

}
#endif /* SRC_UTIL_APPRANDOM_HH_ */
