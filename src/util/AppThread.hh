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
 * AppThread.hh
 *
 *  Created on: 11-Aug-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_APPTHREAD_HH_
#define SRC_UTIL_APPTHREAD_HH_

#include <common.h>
#include <pthread.h>


namespace util {

class AppThread {
public:
    AppThread(appBool free = FALSE);
    virtual ~AppThread();

    static void *runInsideThread(void *data);

    pthread_t start();
    pthread_t start(void *dt);
    virtual void run() {};
    virtual void run(void *dt) {};
    void wait();
private:
    int createThread(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
    appInt running;
    pthread_t tid;
    appBool free; //free at the end
};

} /* namespace util */

#endif /* SRC_UTIL_APPTHREAD_HH_ */
