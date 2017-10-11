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
 * AppMutex.hh
 *
 *  Created on: 24-Aug-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_APPMUTEX_HH_
#define SRC_UTIL_APPMUTEX_HH_
#include <pthread.h>

namespace util{
class AppMutex{
public:
    AppMutex(bool lock = true) : allowLock(lock){pthread_mutex_init(&mutex, NULL);}
    ~AppMutex(){pthread_mutex_destroy(&mutex);}
    void lock();
    void unlock();
private:
    pthread_mutex_t mutex;
    bool allowLock;
};
}

inline void util::AppMutex::lock() {
    if(allowLock) pthread_mutex_lock(&mutex);
}

inline void util::AppMutex::unlock() {
    if(allowLock) pthread_mutex_unlock(&mutex);
}

#endif /* SRC_UTIL_APPMUTEX_HH_ */
