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
 * AppThread.cc
 *
 *  Created on: 11-Aug-2017
 *      Author: abhijit
 */

#include "AppThread.hh"

#include <pthread.h>
#include <cstdlib>

#include <common.h>

namespace util {

struct APP_THREAD_DATA{
    appInt64 count;
    AppThread *self;
    void *data;
};

AppThread::AppThread(appBool free):running(0), tid(0), free(free) {
    // TODO Auto-generated constructor stub

}

AppThread::~AppThread() {
    // TODO Auto-generated destructor stub
}

void* AppThread::runInsideThread(void* data) {
    auto dt = (APP_THREAD_DATA *)data;
    AppThread *self = dt->self;
    auto count = dt->count;
    auto thData = dt->data;
    delete dt;
    switch(count){
    case 0:
        self->run();
        break;
    case 1:
        self->run(thData);
        break;
    default:
        assert(0);
    }
//    self->run();
    self->running --;
    if(self->free and self->running == 0){
        delete self;
    }
    return NULL;
}

pthread_t AppThread::start() {
        auto self = this;
        APP_THREAD_DATA *dt = new APP_THREAD_DATA();
        dt->count = 0;
        dt->self = self;
        dt->data = NULL;
        if(createThread(&tid, NULL, AppThread::runInsideThread, dt)){
            exit(12);
        }
        return tid;
}

pthread_t AppThread::start(void* dt) {
        auto self = this;
//        auto data = APP_PACK(self);
        APP_THREAD_DATA *thrd = new APP_THREAD_DATA();
        thrd->count = 1;
        thrd->self = self;
        thrd->data = dt;
        pthread_t tid;
        if(createThread(&tid, NULL, AppThread::runInsideThread, thrd)){
            exit(12);
        }
        return tid;
}

int AppThread::createThread(pthread_t* thread, const pthread_attr_t* attr,
        void* (*start_routine)(void*), void* arg) {
    running ++;
    auto ret = pthread_create(thread, attr, start_routine, arg);
    if(ret)
        running--;
    return ret;
}

void AppThread::wait() {
    if(!running)
        return;
    pthread_join(tid, NULL);
}

} /* namespace util */
