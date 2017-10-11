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
 * ThreadPool.h
 *
 *  Created on: 29-Mar-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_THREADPOOL_HH_
#define SRC_UTIL_THREADPOOL_HH_

#include <pthread.h>
#include "ConditonalWait.hh"
#include <mutex>
#include <semaphore.h>
#include <set>

#include "AppLLQueue.hh"
#include "AppPool.hh"

namespace util {

typedef void *(*cb_func)(void *data);

struct Job:public LL_Node{
    cb_func func;
    void *data;
    bool terminate;
//    static Job *getJob();
//    static void remJob(Job *);
//private:
    Job():func(NULL), data(NULL), terminate(false){}
    ~Job(){};
};


//AppPool<Job> &getJobPool();


class ThreadPool {
public:
    ThreadPool(int threadCount, bool start = false);
    virtual ~ThreadPool();
    static void *func(void *data);
    void executeInsidePool(cb_func fn, void *data);
    void putJob(Job *jb);
    void run();
    void stop();
    Job *getJob();
    static AppPool<Job> &getJobPool();
    AppLLQueue<Job> jobQueue;
private:
    struct ThreadOfPool{
        ThreadOfPool():t(0), index(0), running(false), jobcount(0){}
        pthread_t t;
        int index;
        bool running;
        int jobcount;
    //    semaphore sem;
    };
    ThreadOfPool *pool;
    int *stack;
    int stackTop;
    int maxCount;
    int poolSize;
    AppSemaphore waitForJob;
    AppSemaphore waitToTerminate;
    int numThreadRunning;
    int turnedOffIndex;
    bool running, stopped;
    util::AppMutex runningLock, getJobLock;
};

class WorkerThread: public ThreadPool{
public:
    WorkerThread(bool start = false):ThreadPool(1, start){}
};


} /* namespace MPIoT */

#endif /* SRC_UTIL_THREADPOOL_HH_ */
