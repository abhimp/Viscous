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
 * AppPool.h
 *
 *  Created on: 31-Mar-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_APPPOOL_H_
#define SRC_UTIL_APPPOOL_H_
#include "AppStack.hpp"
#define PACKETS_PER_POOL 128

#define NO_POOL

template <typename T>
class AppPool {
public:
    AppPool();
    virtual ~AppPool();
    inline T *getNew();
    inline void free(T *);
private:
    T **listing;
    AppStack<T *> pool;
    appSInt poolCapa;
    appSInt poolTop;
    T *localPool;
    appSInt localPoolTop;
    appSInt64 allocatedPacket;
    std::mutex rwlock;
};


template<typename T>
AppPool<T>::AppPool() :listing(NULL), pool(), poolCapa(0), poolTop(-1), localPool(NULL), localPoolTop(0), allocatedPacket(0), rwlock() {

}

template<typename T>
AppPool<T>::~AppPool() {
    LOGE("Objects still in circulations: %ld (%s)", allocatedPacket, __PRETTY_FUNCTION__);
}

template<typename T>
inline T *AppPool<T>::getNew() {
    rwlock.lock();
    T *ele;
#ifndef NO_POOL
    if(localPool == NULL or localPoolTop == PACKETS_PER_POOL)
    {
        if(pool.empty()){
            localPool = new T[PACKETS_PER_POOL];
            localPoolTop = 1;
            ele = &localPool[0];
        }
        else{
            ele = pool.top();
            pool.pop();
        }
    }
    else
    {
        ele = &localPool[localPoolTop];
        localPoolTop += 1;
    }
    APP_ASSERT(ele);
#else
    ele = new T();
#endif
    allocatedPacket ++;

    rwlock.unlock();
    return ele;
}

template<typename T>
inline void AppPool<T>::free(T *ele) {
    rwlock.lock();
#ifndef NO_POOL
    pool.push(ele);
#else
    delete ele;
#endif
    allocatedPacket --;
    rwlock.unlock();
}



#endif /* SRC_UTIL_APPPOOL_H_ */
