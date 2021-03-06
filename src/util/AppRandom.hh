/*
 * This is an implementation of Viscous protocol.
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
