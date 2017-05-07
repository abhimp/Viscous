#include <appThread.h>

void *call_function(void* arg)
{
	appByte *buf = arg;
	appThreadCallBack call_back;
	appByte *extBuf;
	APP_UNPACK(buf, call_back, extBuf);
	call_back(extBuf, pthread_self());
	pthread_exit(NULL);
	return NULL;
}
sem_t finishThreadFromPool;
void *appTerminateThreadFromPool(void *arg, appThreadInfoId tid)
{
    sem_post(&finishThreadFromPool);
    pthread_exit(NULL);
    return NULL;
}

//appStatus runInThread(void *(*call_back)(void *), appByte *extBuf, appBool detachable)
//appStatus runInThread(appThreadCallBack call_back, appByte *extBuf, appBool detachable)
appStatus runInThreadGetTid(appThreadCallBack call_back, appByte *extBuf, appBool detachable, pthread_t *argTid)
{
	pthread_t tid;
	appByte *buf = APP_PACK(call_back, extBuf);

	if(pthread_create(&tid, NULL, call_function, buf) < 0)
	{
		appFree(buf);
		LOGW("thread creation failed");
		APP_RETURN_FAILURE;
	}
	if(detachable)
		pthread_detach(tid);
	if(argTid)
		*argTid = tid;
	APP_RETURN_SUCCESS;
}

appStatus startThread4Pool(appThreadInfo *tInfo, appBool detachable)
{
	pthread_t tid;

	if(pthread_create(&tid, NULL, threadWaitForJob, (appByte *)tInfo) < 0)
	{
		LOGW("thread creation failed");
		APP_RETURN_FAILURE;
	}
    tInfo->tid = tid;
	if(detachable)
		pthread_detach(tid);

    APP_RETURN_SUCCESS;
}

appThreadPoolInfo *initializeThreadPool(appInt maxThreadCnt) // 0 = unlimited
{
    appThreadPoolInfo *poolInfo;
    appInt i = 0;
    APP_ASSERT(maxThreadCnt && maxThreadCnt <= 100);
    poolInfo = appMalloc(sizeof(appThreadPoolInfo));

    poolInfo->maxThreadCnt = maxThreadCnt;
    poolInfo->dynamic = maxThreadCnt == 0 ? TRUE : FALSE;

    poolInfo->infos = appMalloc(sizeof(appThreadInfo) * maxThreadCnt);
    poolInfo->infoPool = appMalloc(sizeof(appInt) * maxThreadCnt);
    poolInfo->infoPoolTop = 0;
    poolInfo->activeThreadCnt = 0;
    poolInfo->detachable = TRUE;
    pthread_mutex_init(&poolInfo->poolManagementLock, NULL);
    sem_init(&poolInfo->waitToFreeUp, 0, 0);
    //sem_init(poolInfo->waitForJob);
    //sem_init(poolInfo->jobRcvd);
    for(i = 0; i < poolInfo->maxThreadCnt; i++)
    {
        sem_init(&poolInfo->infos[i].waitForJob, 0, 0);
        sem_init(&poolInfo->infos[i].jobRcvd, 0, 0);
        poolInfo->infos[i].threadPool = poolInfo;
        poolInfo->infos[i].threadId = i;
        if(startThread4Pool(poolInfo->infos + i, TRUE) != APP_SUCCESS)
        {
            LOGE("Thread creation failed for thread_index %d.", i);
            exit(12);
        }
        poolInfo->infoPool[poolInfo->infoPoolTop] = i;
        poolInfo->infoPoolTop++;
    }
    poolInfo->activeThreadCnt = i;
    return poolInfo;
}

appStatus destroyThreadPool(appThreadPoolInfo *poolInfo) 
{
    APP_ASSERT(poolInfo);

    appInt i;
    appThreadInfo *thread;
    appByte *arg = NULL;
    void *ptr = NULL;
    appThreadCallBack call_back = appTerminateThreadFromPool;
    sem_init(&finishThreadFromPool, 0, 0);
    for(i = 0; i < poolInfo->maxThreadCnt; i++)
    {
        thread = poolInfo->infos + i;
        arg = pack(2,
                PACKIT(call_back),
                PACKIT(ptr)
                );
        thread->buf = arg;
        sem_post(&thread->waitForJob);
        sem_wait(&thread->jobRcvd);
        sem_wait(&finishThreadFromPool);
    }

    appCondFree(poolInfo->infos); 
    appCondFree(poolInfo->infoPool);
    poolInfo->infoPoolTop = 0; 
    appCondFree(poolInfo);

    APP_RETURN_SUCCESS;
}

void *threadWaitForJob(void *buf)
{
    appThreadInfo *tInfo = buf;
    appThreadCallBack call_back;
    appByte *extBuf = NULL;
    while(1)
    {
        sem_wait(&tInfo->waitForJob);
        unpack(tInfo->buf, 2,
                PACKIT(call_back),
                PACKIT(extBuf)
              );
        tInfo->status = APP_SUCCESS;
        sem_post(&tInfo->jobRcvd);
        call_back(extBuf, tInfo->threadId);
        addFreeThread(tInfo);
    }
    return NULL;
}

appStatus addFreeThread(appThreadInfo *threadInfo)
{
    APP_ASSERT(threadInfo);
    appThreadPoolInfo *pool = threadInfo->threadPool;
    
    pthread_mutex_lock(&pool->poolManagementLock);
    APP_ASSERT(pool->infoPoolTop < pool->activeThreadCnt);
    pool->infoPool[pool->infoPoolTop] = threadInfo->threadId;
    if(pool->infoPoolTop == 0)
        sem_post(&pool->waitToFreeUp);
    pool->infoPoolTop++;
    pthread_mutex_unlock(&pool->poolManagementLock);
    APP_RETURN_SUCCESS;
}

appThreadInfo *getFreeThread(appThreadPoolInfo *pool)
{
    APP_ASSERT(pool);
    appThreadInfo *tInfo = NULL;
    appInt index = 0;
    while(1)
    {
        pthread_mutex_lock(&pool->poolManagementLock);
        if(pool->infoPoolTop > 0)
        {
            pool->infoPoolTop--;
            index = pool->infoPool[pool->infoPoolTop];
            tInfo = &pool->infos[index];
        }
        pthread_mutex_unlock(&pool->poolManagementLock);
        if(tInfo)
            return tInfo;
        
        sem_wait(&pool->waitToFreeUp);
    }
    return NULL;
}

appStatus runInThreadPoolGetThreadId(appThreadPoolInfo *threadPool, appThreadCallBack call_back, void *buf, appThreadInfoId *tid)
{
    APP_ASSERT(threadPool);
    appThreadInfo *thread = getFreeThread(threadPool);
    appByte *arg = NULL;
    arg = pack(2,
            PACKIT(call_back),
            PACKIT(buf)
        );
    thread->buf = arg;
    sem_post(&thread->waitForJob);
    sem_wait(&thread->jobRcvd);
    if(tid) *tid = thread->threadId;
    return thread->status;
}
