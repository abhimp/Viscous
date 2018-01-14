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
 * AppLLQueue.h
 *
 *  Created on: 08-Aug-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_APPLLQUEUE_HH_
#define SRC_UTIL_APPLLQUEUE_HH_

#include <common.h>
#include <stddef.h>
#include <cassert>
#include <mutex>
#include <memory>

#include "ConditonalWait.hh"
#include "AppMutex.hh"

namespace util{

template<typename T>
class AppLLQueue;

class LL_Node{
public:
    LL_Node():next(NULL), prev(NULL){}
    LL_Node *next;
    LL_Node *prev;
};

template<typename T>
class AppLLQueue {
public:
    AppLLQueue(int queueSize = -1);
    virtual ~AppLLQueue();

    inline void reset();
    inline appInt addLLToQueue(LL_Node *node);
    inline void addToQueue(LL_Node *node);
    inline T *getFromQueue();
    inline appBool empty();
    inline T *getAllFromQueue();
private:
    inline void add(LL_Node* node);
    LL_Node *beg, *end;
    int queueSize;
    int maxQueueSize;
    util::AppSemaphore limitQueueSize;
    util::AppMutex queueLock;
};


template<typename T>
AppLLQueue<T>::AppLLQueue(int queueSize):beg(NULL), end(NULL), queueSize(0), maxQueueSize(queueSize), limitQueueSize(queueSize>0? queueSize: 0) {
    assert(queueSize);
}

template<typename T>
AppLLQueue<T>::~AppLLQueue() {
    // TODO Auto-generated destructor stub
}

template<typename T>
inline void AppLLQueue<T>::reset() {
    beg = NULL;
    end = NULL;
    queueSize = 0;
}

template<typename T>
inline appInt AppLLQueue<T>::addLLToQueue(LL_Node* node) {
    LL_Node* tmp = node;
    assert(maxQueueSize <= 0);
    appInt cnt = 0;
    queueLock.lock();
    while(tmp){
        auto p = tmp;
        tmp = tmp->next;
        add(p);
        cnt++;
    }
    queueLock.unlock();
    return cnt;
}

template<typename T>
inline void AppLLQueue<T>::add(LL_Node* node) {
    node->next = NULL;
    if(beg != NULL){
        end->next = node;
        end = node;
    }
    else{
        beg = end = node;
    }
    queueSize ++;
}

template<typename T>
inline void AppLLQueue<T>::addToQueue(LL_Node *node) {
    if(maxQueueSize > 0)
        limitQueueSize.wait();
    queueLock.lock();
    add(node);
    queueLock.unlock();
}

template<typename T>
inline T *AppLLQueue<T>::getFromQueue() {
    queueLock.lock();
    if (beg == NULL){
        queueLock.unlock();
        return NULL;
    }
    auto tmp = beg;
    beg = beg->next;
    if(beg == NULL){
        end = NULL;
    }
    queueSize --;
    queueLock.unlock();
    if(maxQueueSize > 0)
        limitQueueSize.notify();
    tmp->next = NULL;
    return (T*)tmp;
}

template<typename T>
inline appBool AppLLQueue<T>::empty() {
    appBool ret = TRUE;
    queueLock.lock();
    if(beg)
        ret = FALSE;
    queueLock.unlock();
    return ret;
}

template<typename T>
inline T* AppLLQueue<T>::getAllFromQueue() {
    if(!beg)
        return NULL;
    queueLock.lock();
    auto tmp = beg;
    beg = end = NULL;
    auto notify = queueSize;
    queueSize = 0;
    queueLock.unlock();
    while(notify and maxQueueSize > 0){
        limitQueueSize.notify();
        notify--;
    }
    return (T*)tmp;
}

////============================================================
//template<typename T>
//class AppSPLLQueue;
//
//class SPLL_Node{
//public:
//    SPLL_Node():next_(NULL), prev_(NULL){}
//
//    SPLL_Node *next_;
//    SPLL_Node *prev_;
//};
//
//template<typename T>
//class AppSPLLQueue {
//public:
//    AppSPLLQueue(int queueSize = -1);
//    virtual ~AppSPLLQueue();
//
//    inline void reset();
//    inline appInt addSPLLToQueue(std::shared_ptr<T> node);
//    inline void addToQueue(std::shared_ptr<T> node);
//    inline std::shared_ptr<T> getFromQueue();
//    inline appBool empty();
//    inline std::shared_ptr<T> getAllFromQueue();
//private:
//    inline void add(std::shared_ptr<T> node);
//    std::shared_ptr<T> beg, end;
//    int queueSize;
//    int maxQueueSize;
//    util::AppSemaphore limitQueueSize;
//    util::AppMutex queueLock;
//};
//
//
//template<typename T>
//AppSPLLQueue<T>::AppSPLLQueue(int queueSize):queueSize(0), maxQueueSize(queueSize), limitQueueSize(queueSize>0? queueSize: 0) {
//    assert(queueSize);
//}
//
//template<typename T>
//AppSPLLQueue<T>::~AppSPLLQueue() {
//    // TODO Auto-generated destructor stub
//}
//
//template<typename T>
//inline void AppSPLLQueue<T>::reset() {
//    beg = NULL;
//    end = NULL;
//    queueSize = 0;
//}
//
//template<typename T>
//inline appInt AppSPLLQueue<T>::addSPLLToQueue(std::shared_ptr<T> node) {
//    std::shared_ptr<T> tmp = node;
//    assert(maxQueueSize <= 0);
//    appInt cnt = 0;
//    queueLock.lock();
//    while(tmp){
//        auto p = tmp;
//        tmp = tmp->next();
//        add(p);
//        cnt++;
//    }
//    queueLock.unlock();
//    return cnt;
//}
//
//template<typename T>
//inline void AppSPLLQueue<T>::add(std::shared_ptr<T> node) {
//    node->next() = NULL;
//    if(beg != NULL){
//        end->next() = node;
//        end = node;
//    }
//    else{
//        beg = end = node;
//    }
//    queueSize ++;
//}
//
//template<typename T>
//inline void AppSPLLQueue<T>::addToQueue(std::shared_ptr<T> node) {
//    if(maxQueueSize > 0)
//        limitQueueSize.wait();
//    queueLock.lock();
//    add(node);
//    queueLock.unlock();
//}
//
//template<typename T>
//inline std::shared_ptr<T> AppSPLLQueue<T>::getFromQueue() {
//    queueLock.lock();
//    if (beg == NULL){
//        queueLock.unlock();
//        return NULL;
//    }
//    auto tmp = beg;
//    beg = beg->next();
//    if(beg == NULL){
//        end = NULL;
//    }
//    queueSize --;
//    queueLock.unlock();
//    if(maxQueueSize > 0)
//        limitQueueSize.notify();
//    tmp->next() = NULL;
//    return tmp;
//}
//
//template<typename T>
//inline appBool AppSPLLQueue<T>::empty() {
//    appBool ret = TRUE;
//    queueLock.lock();
//    if(beg)
//        ret = FALSE;
//    queueLock.unlock();
//    return ret;
//}
//
//template<typename T>
//inline std::shared_ptr<T> AppSPLLQueue<T>::getAllFromQueue() {
//    if(!beg)
//        return NULL;
//    queueLock.lock();
//    auto tmp = beg;
//    beg = end = NULL;
//    auto notify = queueSize;
//    queueSize = 0;
//    queueLock.unlock();
//    while(notify and maxQueueSize > 0){
//        limitQueueSize.notify();
//        notify--;
//    }
//    return tmp;
//}

}

#endif /* SRC_UTIL_APPLLQUEUE_HH_ */
