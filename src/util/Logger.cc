/*
 * This is an implemetation of Viscous protocol.
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
 * Logger.cc
 *
 *  Created on: 05-Mar-2017
 *      Author: abhijit
 */

#include "Logger.h"

namespace LOGGER {

Logger::Logger():fileOpened(FALSE), fp(stdout) {
}

Logger::Logger(char* fpath):fileOpened(FALSE), fp(stdout) {
    FILE *p = fopen(fpath, "w");
    if(p){
        fp = p;
        fileOpened = TRUE;
    }
}

Logger::~Logger() {
    if(fileOpened){
        writelock.lock();
        fflush(fp);
        fclose(fp);
        writelock.unlock();
        fp = stderr;
        fileOpened = FALSE;
    }
}

} /* namespace ARQ */
