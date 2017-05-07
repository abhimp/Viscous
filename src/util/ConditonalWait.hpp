/*
 * ConditonalWait.hpp
 *
 *  Created on: 20-Feb-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_CONDITONALWAIT_HPP_
#define SRC_UTIL_CONDITONALWAIT_HPP_
#include <semaphore.h>

class ConditionalWait{
    sem_t sem1, sem2;
    int flag;
public:
    ConditionalWait(){
        sem_init(&sem1, 0, 1);
        sem_init(&sem2, 0, 0);
        flag = 0;
    }
    ~ConditionalWait(){
        sem_destroy(&sem1);
        sem_destroy(&sem2);
    }
    inline void wait(){
        sem_wait(&sem1);
        flag ++;
        sem_post(&sem1);
        sem_wait(&sem2);
    }
    inline void notify(){
        sem_wait(&sem1);
        if(flag > 0){
            flag -= 1;
            sem_post(&sem2);
        }
        sem_post(&sem1);
    }
};

struct AppSemaphore{
private:
    sem_t sem1;
public:
    AppSemaphore(unsigned int initVal=0){
        sem_init(&sem1, 0, initVal);
    }
    ~AppSemaphore(){
        sem_destroy(&sem1);
    }
    void wait(){
        sem_wait(&sem1);
    }
    void notify(){
        sem_post(&sem1);
    }
};

#endif /* SRC_UTIL_CONDITONALWAIT_HPP_ */
