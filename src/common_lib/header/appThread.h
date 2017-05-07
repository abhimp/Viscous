
/*
 * appThread.h
 *
 *  Created on: 16-Feb-2016
 *      Author: abhijit
 */

#ifndef __APP_THREAD_H_
#define __APP_THREAD_H_
#include <common.h>
#include <pthread.h>
#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __app_thread_info__{
	pthread_t tid;
    sem_t waitForJob;
    sem_t jobRcvd;
    appByte *buf;
    appStatus status;
    void *threadPool;
    appInt threadId;
}appThreadInfo;

typedef struct __thread_pool_info__{
    appBool dynamic;
    appInt maxThreadCnt;
    appThreadInfo *infos;
    appInt activeThreadCnt;
    appInt *infoPool;
    appInt infoPoolTop; //0 == empty, maxThreadCnt == full
    appBool detachable;
    pthread_mutex_t poolManagementLock;
    sem_t waitToFreeUp;
}appThreadPoolInfo;


typedef pthread_t appThreadInfoId;
typedef void *(*appThreadCallBack)(void *, appThreadInfoId tid);

#define runInThreadDetachable(_x, _y) runInThread(_x, _y, TRUE)
#define runInThread(_x, _y, _z) runInThreadGetTid(_x, _y, _z, NULL)
#define runInThreadPool(_x, _y, _z) runInThreadPoolGetThreadId(_x, _y, _z, NULL)

void *call_function(void* arg);
appStatus runInThreadGetTid(appThreadCallBack call_back, appByte *extBuf, appBool detachable, pthread_t *argTid);
appStatus startThread4Pool(appThreadInfo *tInfoi, appBool detachable);
appThreadPoolInfo *initializeThreadPool(appInt maxThreadCnt); // 0 = unlimited
appStatus destroyThreadPool(appThreadPoolInfo *poolInfo);
void *threadWaitForJob(void *buf);
appStatus runInThreadPoolGetThreadId(appThreadPoolInfo *threadPool, appThreadCallBack call_back, void *buf, appThreadInfoId *tid);
appStatus addFreeThread(appThreadInfo *threadInfo);
appThreadInfo *getFreeThread(appThreadPoolInfo *pool);


#ifdef __cplusplus
}
#endif

#endif
