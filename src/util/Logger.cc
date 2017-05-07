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
