/*
 * ThreadPool.cpp
 *
 *  Created on: 29-Mar-2017
 *      Author: abhijit
 */

//#include <iostream>
#include <common.h>

#include "ThreadPool.h"

namespace UTIL {

ThreadPool::ThreadPool(int threadCount, bool start) : pool(NULL), stack(NULL), stackTop(-1), maxCount(threadCount), poolSize(0), numThreadRunning(0), turnedOffIndex(-1), running(false), stopped(false), APP_LL_QUEUE_INIT_LIST(Job)
{
    if(start){
        run();
    }
}

ThreadPool::~ThreadPool() {
    stop();
}

void* ThreadPool::func(void* data) {
    ThreadPool *self;
    ThreadOfPool *thrd;
    APP_UNPACK((appByte *)data, self, thrd);
    thrd->running = true;
    while(thrd->running){
        auto jb = self->getJob();
        if(jb == NULL)
            continue;
        if(jb->terminate){
            getJobPool().free(jb);
            thrd->running = false;
            self->turnedOffIndex = thrd->index;
            self->numThreadRunning --;
            self->waitToTerminate.notify();
            pthread_exit(NULL);
            break;
        }
        thrd->jobcount ++;
//        LOGI("Thread %d, running %d", thrd->index, thrd->jobcount);
        jb->func(jb->data);
        getJobPool().free(jb);
//        LOGI("Thread %d, finishing %d", thrd->index, thrd->jobcount);
    }
    thrd->running = false;
//    LOGI("exiting %d", thrd->index);
//    std::cout << "exiting" << thrd->index << std::endl;
    return NULL;
}

void ThreadPool::run() {
    if(running or stopped) return;
    runningLock.lock();
    pool = new ThreadOfPool[maxCount];
    stack = new int[maxCount];
    running = true;
    auto self = this;
    for(auto i = 0; i < maxCount; i++, poolSize++){
        pool[i].index = i;
        auto x = &pool[i];
        auto data = APP_PACK(self, x);
        if(pthread_create(&pool[i].t, NULL, ThreadPool::func, data)){
            exit(12);
        }
        pool[i].running = true;
        numThreadRunning ++;
    }
    runningLock.unlock();
}

void ThreadPool::executeInsidePool(cb_func fn, void* data) {
    if(!running) return;
    Job *jb = getJobPool().getNew(); //new Job();
    jb->func=fn;
    jb->data = data;
    putJob(jb);
//    LOGE("job count:%d", JobQueueSize);
}

void ThreadPool::putJob(Job* jb) {
    getJobLock.lock();
    addToQueueJob(jb);
    waitForJob.notify();
    getJobLock.unlock();
}

void ThreadPool::stop() {
    runningLock.lock();
    running = false;
    stopped = true;
//    LOGE("inside pool stop: queusize: %d poolsize: %d", JobQueueSize, poolSize);
    for(int i = 0; i < poolSize; i++){
//        LOGE("running: %d", pool[i].running);
        if(!pool[i].running) continue;

        auto jb = getJobPool().getNew(); //new Job();
        jb->terminate = true;
        runningLock.unlock();
        turnedOffIndex = -1;
        putJob(jb);
        waitToTerminate.wait();
//        LOGE("Killing:%d queue:%d", turnedOffIndex, JobQueueSize);
        pthread_join(pool[turnedOffIndex].t, NULL);
        runningLock.lock();
    }

    runningLock.unlock();
}

Job* ThreadPool::getJob() {
//    getJobLock.lock();
    Job *jb = NULL;
//    while(JobQueueSize == 0){
//        getJobLock.unlock();
        waitForJob.wait();
//        getJobLock.lock();
//    }
    getJobLock.lock();
    jb = getFromQueueJob();
    getJobLock.unlock();
    return jb;
}

AppPool<Job>& ThreadPool::getJobPool() {
    static AppPool<Job> jobPool;
    return jobPool;
}


} /* namespace MPIoT */
