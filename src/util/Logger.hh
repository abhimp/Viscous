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
 * Logger.h
 *
 *  Created on: 05-Mar-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_LOGGER_HH_
#define SRC_UTIL_LOGGER_HH_
#include <common.h>
#include <stdio.h>
#include <stdarg.h>
#include "AppMutex.hh"

#define APP_LOG_WRITE(_obj, _f, _l, _lvl, frmt, ...) _obj.writelog(_f ":" APP_CONVERT_TO_STRING(_l) "::" _lvl ":: " frmt "\n", ## __VA_ARGS__)

#if APP_LOG_PRIORITY <= APP_LOG_VERBOSE
#define  LOG_WRITE_V(obj_, ...) { APP_LOG_WRITE(obj_, __FILE__, __LINE__, "VERBOSE",  ## __VA_ARGS__); }
#else
#define  LOG_WRITE_V(...)
#endif
#if APP_LOG_PRIORITY <= APP_LOG_DEBUG
#define  LOG_WRITE_D(obj_, ...) { APP_LOG_WRITE(obj_, __FILE__, __LINE__, "DEBUG",  ## __VA_ARGS__); }
#else
#define  LOG_WRITE_D(obj_, ...)
#endif
#if APP_LOG_PRIORITY <= APP_LOG_INFO
#define  LOG_WRITE_I(obj_, ...) { APP_LOG_WRITE(obj_, __FILE__, __LINE__,  "INFO", ## __VA_ARGS__); }
#else
#define  LOG_WRITE_I(...)
#endif
#if APP_LOG_PRIORITY <= APP_LOG_WARN
#define  LOG_WRITE_W(obj_, ...) { APP_LOG_WRITE(obj_, __FILE__, __LINE__, "WARN", ## __VA_ARGS__); }
#else
#define  LOG_WRITE_W(...)
#endif
#if APP_LOG_PRIORITY <= APP_LOG_ERROR
#define  LOG_WRITE_E(obj_, ...) { APP_LOG_WRITE(obj_, __FILE__, __LINE__, "ERROR", ## __VA_ARGS__); }
#else
#define  LOG_WRITE_E(...)
#endif
#if APP_LOG_PRIORITY <= APP_LOG_FATAL
#define  LOG_WRITE_F(obj_, ...) { APP_LOG_WRITE(obj_, __FILE__, __LINE__, "FATAL", ## __VA_ARGS__); }
#else
#define  LOG_WRITE_F(...)
#endif

namespace LOGGER {

class Logger {
    appBool fileOpened;
    util::AppMutex writelock;
    FILE *fp;
public:
    Logger();
    Logger(char *fpath);
    inline void writelog(const char *frmt, ...){va_list ap; writelock.lock(); va_start(ap, frmt); vfprintf(fp, frmt, ap); va_end(ap); writelock.unlock();}
    virtual ~Logger();
};

} /* namespace FlowHandler */

#endif /* SRC_UTIL_LOGGER_HH_ */
