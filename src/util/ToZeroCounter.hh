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
 * ToZeroCounter.h
 *
 *  Created on: 18-Apr-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_TOZEROCOUNTER_HH_
#define SRC_UTIL_TOZEROCOUNTER_HH_
#include "ConditonalWait.hh"
#include <mutex>
namespace util{
template<typename T>
class ToZeroCounter{
public:
    ToZeroCounter():counter(0), waiting(0){};
    void operator++(int);
    void operator--(int);
    void waitForZero();
private:
    T counter;
    util::ConditionalWait sem;
    int waiting;
    util::AppMutex lock1, lock2;
};

template<typename T>
inline void ToZeroCounter<T>::operator ++(int) {
    lock1.lock();
    counter++;
    lock1.unlock();
}

template<typename T>
inline void ToZeroCounter<T>::operator --(int) {
    lock1.lock();
    counter--;
    if(counter == 0){
        while(waiting){
            sem.notify();
            waiting--;
        }
    }
    lock1.unlock();

}

template<typename T>
inline void ToZeroCounter<T>::waitForZero() {
    lock1.lock();
    if(counter==0){
        lock1.unlock();
        return;
    }
    waiting++;
    lock1.unlock();
    sem.wait();
}
}
#endif /* SRC_UTIL_TOZEROCOUNTER_HH_ */
