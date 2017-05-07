/*
 * common.c
 *
 *  Created on: 12-Feb-2016
 *      Author: abhijit
 */
#include <common.h>
#include <pthread.h>
appFile appFileOpen(appString fname, appFileOperationMode mode){
	appChar *opMode;
	FILE *fp = NULL;
	appFile file = NULL;
	switch(mode)
	{
		case APP_FILE_MODE_READ:
			opMode = "r";
			break;
		case APP_FILE_MODE_WRITE:
			opMode = "w";
			break;
		default:
			return NULL;
	}
	fp = fopen(fname, opMode);
	
	if(fp == NULL){
		LOGV("fp is not null");
		return NULL;
	}

	file = appCalloc(sizeof(appFileOperation), 1);
	file->fname = appMalloc(strlen(fname) + 1);
	strcpy(file->fname, fname);
	file->mode = mode;
	file->fp = fp;

	return file;
}

void appFileClose(appFile file){
	if(file->fp)
		fclose(file->fp);
	appFree(file->fname);
	appFree(file);
}

pthread_mutex_t APP_MEMEORY_LOCK = PTHREAD_MUTEX_INITIALIZER;

#ifdef __APP_THREAD_SAFE_MEMORY__

void *appMallocWrapper(size_t size)
{
	void *tmpPtr;
	pthread_mutex_lock(&APP_MEMEORY_LOCK);
	tmpPtr = malloc(size);
	pthread_mutex_unlock(&APP_MEMEORY_LOCK);
	if(!tmpPtr) LOGF("Allocation failed for size %lu.", size);
	return tmpPtr;
}
void appFreeWrapper(void *ptr)
{
	pthread_mutex_lock(&APP_MEMEORY_LOCK);
	free(ptr);
	pthread_mutex_unlock(&APP_MEMEORY_LOCK);
}
void *appCallocWrapper(size_t nmemb, size_t size)
{
	void *tmpPtr;
	pthread_mutex_lock(&APP_MEMEORY_LOCK);
	tmpPtr = calloc(nmemb, size);
	pthread_mutex_unlock(&APP_MEMEORY_LOCK);
	if(!tmpPtr) LOGF("Allocation failed for size %lu.", size);
	return tmpPtr;
}
void *appReallocWrapper(void *ptr, size_t size)
{
	void *tmpPtr;
	pthread_mutex_lock(&APP_MEMEORY_LOCK);
	tmpPtr = realloc(ptr, size);
	pthread_mutex_unlock(&APP_MEMEORY_LOCK);
	if(!tmpPtr) LOGF("Allocation failed for size %lu.", size);
	return tmpPtr;
}

appStatus appCreateNCopyStr(appString *dst, appString src)
{
	appInt len = strlen(src);
	if(!len)
		APP_RETURN_FAILURE
	*dst = appMalloc(len+1);
	strncpy(*dst, src, len+1);
	APP_RETURN_SUCCESS
}

appByte *pack(appInt cnt, size_t size, ...){
	va_list ap, aq;
	appInt i;
	appInt totsize = size;
	size_t tmpS;
	void *tmpP;
	va_start(ap, size);
	va_copy(aq, ap);

	appByte *buf, *tmp;

//	for(i = 0; i < cnt; i++)
//	{
//		tmpS = va_arg(ap, size_t);
//		totsize +=tmpS;
//		tmpP = va_arg(ap, void *);
//	}
	
	tmp = buf = appMalloc(totsize);
	if(!buf)
		return buf;
	
	for(i = 0; i < cnt; i++)
	{
		tmpS = va_arg(aq, size_t);
		tmpP = va_arg(aq, void *);
		memcpy(tmp, tmpP, tmpS);
		tmp += tmpS;
	}
	va_end(aq);
	va_end(ap);
	return buf;
}

void unpack(appByte *buf, appInt cnt, size_t size, ...){
	va_list ap;
	appInt i;
	size_t tmpS;
	void *tmpP;
	va_start(ap, size);
	appByte *tmp = buf;
	
	if(!buf)
		return;
	
	for(i = 0; i < cnt; i++)
	{
		tmpS = va_arg(ap, size_t);
		tmpP = va_arg(ap, void *);
		memcpy(tmpP, tmp, tmpS);
		tmp += tmpS;
	}
	va_end(ap);
	appFree(buf);
}
#endif
