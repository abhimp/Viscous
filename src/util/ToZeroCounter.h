/*
 * ToZeroCounter.h
 *
 *  Created on: 18-Apr-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_TOZEROCOUNTER_H_
#define SRC_UTIL_TOZEROCOUNTER_H_
#include "ConditonalWait.hpp"
#include <mutex>

template<typename T>
class ToZeroCounter{
public:
    ToZeroCounter():counter(0), waiting(0){};
    void operator++(int);
    void operator--(int);
    void waitForZero();
private:
    T counter;
    ConditionalWait sem;
    int waiting;
    std::mutex lock1, lock2;
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

#endif /* SRC_UTIL_TOZEROCOUNTER_H_ */
