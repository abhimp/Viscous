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
 * common.h
 *
 *  Created on: 12-Feb-2016
 *      Author: abhijit
 */

#ifndef COMMON_H_
#define COMMON_H_
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "common_macros.h"
//#if _POSIX_C_SOURCE >= 199309L
#if 1
#include <time.h>
#else
#include <sys/time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Macros */
#define __APP_THREAD_SAFE_MEMORY__
#ifndef __APP_THREAD_SAFE_MEMORY__

#define appMalloc(size)     malloc(size)
#define appFree(ptr)         free(ptr)
#define appCalloc(nmemb, size)     calloc(nmemb, size)
#define appRealloc(ptr, size)     realloc(ptr, size)
#else

#define appMalloc(size)     appMallocWrapper(size)
#define appFree(ptr)         appFreeWrapper(ptr)
#define appCalloc(nmemb, size)     appCallocWrapper(nmemb, size)
#define appRealloc(ptr, size)     appReallocWrapper(ptr, size)

void *appMallocWrapper(size_t size);
void appFreeWrapper(void *ptr);
void *appCallocWrapper(size_t nmemb, size_t size);
void *appReallocWrapper(void *ptr, size_t size);

#endif

#define appCondFree(ptr)        {if(ptr) {appFree((ptr)); (ptr) = NULL;}}

#define TRUE     1
#define FALSE     0


#define APP_ASSERT(_expr) APP_ASSERT_MSG(_expr, "NO MSG")
#define APP_ASSERT_MSG(_expr, _msg) { \
            if(!(_expr)){ \
                LOGE("APP_ASERT FAILED: %s, Assert msg: %s", APP_CONVERT_TO_STRING(_expr), _msg); \
                assert(_expr); \
            } \
            }
#define PACKIT(_x) sizeof(_x), &(_x)
#define PACKETIT(_x) , PACKIT(_x)
#define PACKETSUM(_x) + sizeof(_x)
#define APP_PACK_(N, ...)     pack(N, APP_MACRO_FOR_EACH_##N(PACKETSUM, PACKETIT, __VA_ARGS__))
#define APP_PACK_N(...) APP_PACK_(__VA_ARGS__)
#define APP_PACK(...)     APP_PACK_N(PP_NARG(__VA_ARGS__), __VA_ARGS__)

#define APP_UNPACK_(_x, N, ...)     unpack(_x, N, APP_MACRO_FOR_EACH_##N(PACKETSUM, PACKETIT, __VA_ARGS__))
#define APP_UNPACK_N(...) APP_UNPACK_(__VA_ARGS__)
#define APP_UNPACK(_x, ...)     APP_UNPACK_N(_x, PP_NARG(__VA_ARGS__), __VA_ARGS__)

#define UPACK(...)


#define APP_MAKE_ENUM_ELE(_x) _x
#define APP_MAKE_STR_ELE(_x) #_x
#define APP_MAKE_BITMAP_ENUM_ELE(_x) _x##_BITMAP = (1<<_x)
#define APP_MAKE_BITMAP_ENUM_ELE_VAL(_x, _v) _x = (1<<_v)
#define APP_MAKE_ENUM_WITH_PREFIX(_pref, _x) _pref##_x


/* no macros any more */



/* typedefs */
//data types
typedef int8_t        appSByte;
typedef int8_t        appChar;
typedef int8_t        appSChar;
typedef int16_t        appSShort;
typedef int32_t        appSInt;
typedef int64_t        appSLong;

typedef uint8_t        appUChar;
typedef uint8_t        appByte;
typedef uint16_t    appShort;
typedef uint32_t    appInt;
typedef uint64_t    appLong;

typedef int8_t      appSInt8;
typedef int16_t     appSInt16;
typedef int32_t     appSInt32;
typedef int64_t     appSInt64;

typedef uint8_t     appInt8;
typedef uint16_t    appInt16;
typedef uint32_t    appInt32;
typedef uint64_t    appInt64;

typedef appChar*     appString;
typedef appByte     appBool;

/* no typedef below this points */

/* enums */
typedef enum _app_file_operation_mode_{
    APP_FILE_MODE_INVALID,
    APP_FILE_MODE_READ,
    APP_FILE_MODE_WRITE,
    /* */
    APP_TOTAL_FILE_MODE
} appFileOperationMode;

typedef enum __aap_status__{
    APP_SUCCESS,
    APP_FAILURE,
} appStatus;
//#if _POSIX_C_SOURCE >= 199309L
#if 0
typedef struct timespec appTime;
#else
typedef struct _app_time_{
    appInt64   tv_sec;
    appInt64  tv_nsec;
}appTime;
#endif
/* stop enum */

/* struct */

typedef struct _file_operation_{
    appString fname;
    FILE *fp;
    appFileOperationMode mode;
    //appByte[2048] buffer;
    //appSInt bufIndex
}appFileOperation;
typedef appFileOperation* appFile;
/* no struct any more */

/* declaration */
appFile appFileOpen(appString fname, appFileOperationMode mode);
void appFileClose(appFile file);
appStatus appCreateNCopyStr(appString *dst, appString src);

appByte *pack(appInt cnt, size_t size, ...);
void unpack(appByte *buf, appInt cnt, size_t size, ...);

/* declaration end */
#define UNIQ_VAR_I(a, b) UNIQ_VAR_II(a, b)
#define UNIQ_VAR_II(a, b) UNIQ_VAR_III(~, a ## b)
#define UNIQ_VAR_III(p, res) res
#define UNIQ_VAR(p) UNIQ_VAR_I(p, __LINE__)

//#if _POSIX_C_SOURCE >= 199309L
#if 1
#define appGetSysTime(x) {struct timespec y; clock_gettime(CLOCK_REALTIME, (&y)); (x)->tv_sec = y.tv_sec; (x)->tv_nsec = y.tv_nsec; }
#else
#define appGetSysTime(x) struct timeval UNIQ_VAR(appTime_); gettimeofday(&UNIQ_VAR(appTime_), NULL); (x)->tv_sec = UNIQ_VAR(appTime_).tv_sec; (x)->tv_nsec = UNIQ_VAR(appTime_).tv_usec*1000;
#endif


#ifdef __ANDROID__
#include <jni.h>
#include <android/log.h>
#define  LOG_TAG    "someTag"

#define  LOGE(...) APP_LOG(ANDROID_LOG_ERROR, __FILE__, __LINE__,  ## __VA_ARGS__)
#define  LOGW(...) APP_LOG(ANDROID_LOG_WARN, __FILE__, __LINE__,  ## __VA_ARGS__)
#define  LOGD(...) APP_LOG(ANDROID_LOG_DEBUG, __FILE__, __LINE__,  ## __VA_ARGS__)
#define  LOGI(...) APP_LOG(ANDROID_LOG_INFO, __FILE__, __LINE__,  ## __VA_ARGS__)
#define  LOGV(...) APP_LOG(ANDROID_LOG_VERBOSE , __FILE__, __LINE__,  ## __VA_ARGS__)
#define  LOGF(...) APP_LOG(ANDROID_LOG_FATAL, __FILE__, __LINE__,  ## __VA_ARGS__)

#define APP_LOG(_x, _f, _l, frmt, ...) __android_log_print(_x, _f ":" APP_CONVERT_TO_STRING(_l), frmt, ## __VA_ARGS__)

#else
//typedef enum _app_log_priority_ {
//    APP_LOG_UNKNOWN = 0,
//    APP_LOG_DEFAULT,    /* only for SetMinPriority() */
//    APP_LOG_VERBOSE,
//    APP_LOG_DEBUG,
//    APP_LOG_INFO,
//    APP_LOG_WARN,
//    APP_LOG_ERROR,
//    APP_LOG_FATAL,
//    APP_LOG_SILENT,     /* only for SetMinPriority(); must be last */
//    APP_PRIORITY_COUNT
//} appLogPriority;
#define APP_LOG_UNKNOWN 0
#define APP_LOG_DEFAULT 1    /* only for SetMinPriority() */
#define APP_LOG_VERBOSE 2
#define APP_LOG_DEBUG 3
#define APP_LOG_INFO 4
#define APP_LOG_WARN 5
#define APP_LOG_ERROR 6
#define APP_LOG_FATAL 7
#define APP_LOG_SILENT 8     /* only for SetMinPriority(); must be last */
#define APP_PRIORITY_COUNT 9

#ifndef APP_LOG_PRIORITY
#define APP_LOG_PRIORITY APP_LOG_DEBUG
//#elif APP_LOG_PRIORITY >= APP_PRIORITY_COUNT
//#undef APP_LOG_PRIORITY
//#define APP_LOG_PRIORITY APP_LOG_INFO
#endif

#define APP_LOG(_x, _f, _l, _lvl, frmt, ...) fprintf(_x, _f ":" APP_CONVERT_TO_STRING(_l) "::" _lvl ":: " frmt "\n", ## __VA_ARGS__)

#if APP_LOG_PRIORITY <= APP_LOG_VERBOSE
#define  LOGV(...) { APP_LOG(stderr, __FILE__, __LINE__, "VERBOSE",  ## __VA_ARGS__); }
#else
#define  LOGV(...) 
#endif
#if APP_LOG_PRIORITY <= APP_LOG_DEBUG
#define  LOGD(...) { APP_LOG(stderr, __FILE__, __LINE__, "DEBUG",  ## __VA_ARGS__); }
#else
#define  LOGD(...) 
#endif
#if APP_LOG_PRIORITY <= APP_LOG_INFO
#define  LOGI(...) { APP_LOG(stdout, __FILE__, __LINE__,  "INFO", ## __VA_ARGS__); }
#else
#define  LOGI(...) 
#endif
#if APP_LOG_PRIORITY <= APP_LOG_WARN
#define  LOGW(...) { APP_LOG(stderr, __FILE__, __LINE__, "WARN", ## __VA_ARGS__); }
#else
#define  LOGW(...) 
#endif
#if APP_LOG_PRIORITY <= APP_LOG_ERROR   
#define  LOGE(...) { APP_LOG(stderr, __FILE__, __LINE__, "ERROR", ## __VA_ARGS__); }
#else
#define  LOGE(...) 
#endif
#if APP_LOG_PRIORITY <= APP_LOG_FATAL   
#define  LOGF(...) { APP_LOG(stdout, __FILE__, __LINE__, "FATAL", ## __VA_ARGS__); }
#else
#define  LOGF(...) 
#endif
#endif
#define APP_CONVERT_TO_STRING(x) #x
#define APP_AS_IT_IS(x) x

#define APP_RETURN_FAILURE { LOGV("%s: Returning failure", __func__); return APP_FAILURE; }
#define APP_RETURN_SUCCESS { LOGV("%s: Returning success", __func__); return APP_SUCCESS; }


#define APP_INT_MIN   (0)
#define APP_INT8_MAX  (255)
#define APP_INT16_MAX (65535)
#define APP_INT32_MAX (4294967295)
#define APP_INT64_MAX (18446744073709551615L)

#define APP_INT8_CNT  (256)
#define APP_INT16_CNT (65536)
#define APP_INT32_CNT (4294967296L)

#define  APP_SINT8_MIN (-128)
#define APP_SINT16_MIN (-32768)
#define APP_SINT32_MIN (-2147483648)
#define APP_SINT64_MIN (-9223372036854775808L)

#define  APP_SINT8_MAX (127)
#define APP_SINT16_MAX (32767)
#define APP_SINT32_MAX (2147483647)
#define APP_SINT64_MAX (9223372036854775807L)

#define APP_POSITIVE_MOD(x_, mod_) (((x_)%(mod_)+(mod_))%(mod_))

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H_ */
