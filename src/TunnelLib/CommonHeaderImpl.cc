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
 * CommonHeaderImpl.cc
 *
 *  Created on: 12-Aug-2016
 *      Author: abhijit
 */

#include "CommonHeaders.hh"
#include <appThread.h>
#include <algorithm>
#include <cstring>

bool appTs::operator> (appTs nts){
    if(nts.sec() != ts.tv_sec)
        return nts.sec() < ts.tv_sec;
    if(nts.nsec() != ts.tv_nsec)
        return nts.nsec() < ts.tv_nsec;
    return false;
}

bool appTs::operator<(appTs nts){
    return nts > *this;
}

appTs appTs::operator+(double n){
    appTs nts;
    nts.nsec() = ((long)(ts.tv_nsec + (n - (double)((long)n) ) * 1000000000L));
    nts.sec() = ts.tv_sec + (time_t)n + nts.nsec()/1000000000L;
    nts.nsec() %= 1000000000L;
    return nts;
}

appTs appTs::operator+(appTs oth){
    appTs nts;
    nts.nsec() += oth.nsec();
    nts.sec() += oth.sec() + nts.nsec()/1000000000L;
    nts.nsec() %= 1000000000L;
    return nts;
}
appTs appTs::addmili(appInt64 ms){
    appTs nts;
    appInt64 ns = ms * 1000000L;
    nts.nsec() = ((long)(ts.tv_nsec + ns));
    nts.sec() = ts.tv_sec + nts.nsec()/1000000000L;
    nts.nsec() %= 1000000000L;
    return nts;
}
appTs appTs::addmicro(appInt64 us){
    appTs nts;
    appInt64 ns = us * 1000L;
    nts.nsec() = ((long)(ts.tv_nsec + ns));
    nts.sec() = ts.tv_sec + nts.nsec()/1000000000L;
    nts.nsec() %= 1000000000L;
    return nts;
}
appTs appTs::addnano(appInt64 ns){
    appTs nts;
    nts.nsec() = ((long)(ts.tv_nsec + ns));
    nts.sec() = ts.tv_sec + nts.nsec()/1000000000L;
    nts.nsec() %= 1000000000L;
    return nts;
}

appSInt64 appTs::getMicro(){
    appSInt64 nts=0;
    nts = sec() * 1000000L + nsec() /1000;
    return nts;
}

appSInt64 appTs::getMili(){
    appSInt64 nts=0;
    nts = sec() * 1000L + nsec() /1000000;
    return nts;
}

std::ostream& operator<< (std::ostream& os, appTs ts){
    os << ts.sec() << "." << ts.nsec()/1000000000.0;
    return os;
}

BaseReliableObj::BaseReliableObj(BaseReliableObj *parent):parent(parent){
    id = getNextChildId();
}

BaseReliableObj::~BaseReliableObj(){
    pthread_mutex_lock(&newIdLock);
    BaseReliableObj::childIdFreePool.insert(id);
    BaseReliableObj::childIdPool.erase(id);
    pthread_mutex_unlock(&newIdLock);
}

appInt BaseReliableObj::getNextChildId(){
    pthread_mutex_lock(&newIdLock);
    appInt newId;
    while(1){
        if(BaseReliableObj::childIdFreePool.empty()){
            nextChildId ++;
            newId = nextChildId;
        }
        else{
            std::set<appInt>::iterator it = BaseReliableObj::childIdFreePool.begin();
            newId = *it;
            BaseReliableObj::childIdFreePool.erase(newId);
        }
        if(BaseReliableObj::childIdPool.find(newId) != BaseReliableObj::childIdPool.end()){
            continue;
        }
        BaseReliableObj::childIdPool.insert(newId);
        break;
    }
    pthread_mutex_unlock(&newIdLock);
    return newId;
}

appInt BaseReliableObj::nextChildId = 0;
pthread_mutex_t BaseReliableObj::newIdLock = PTHREAD_MUTEX_INITIALIZER;
std::set<appInt> BaseReliableObj::childIdFreePool;
std::set<appInt> BaseReliableObj::childIdPool;

void TimeOutProducer::attach(std::shared_ptr<TimeoutObserver> rm){
    accessMutex.lock();
    if(!hasKey(listner, rm) and !hasKey(remove, rm))
        this->added.insert(rm);
    accessMutex.unlock();
}

void TimeOutProducer::detach(std::shared_ptr<TimeoutObserver> rm){
    accessMutex.lock();
    if(hasKey(listner, rm) and !hasKey(added, rm))
        remove.insert(rm);
    accessMutex.unlock();
}

void TimeOutProducer::timeoutEvent(appTs time){
    accessMutex.lock();
    for(auto x: remove){
        listner.erase(x);
    }
    for(auto x:added){
        listner.insert(x);
    }
    remove.clear();
    added.clear();
    accessMutex.unlock();
    for(auto rm : listner){
        rm->timeoutEvent(time);
    }
}

