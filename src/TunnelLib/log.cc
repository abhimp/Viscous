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
 * log.cc
 *
 *  Created on: 17-Jan-2017
 *      Author: sourav
 */
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "log.hpp"
#include <mutex>

#define DEBUG
#define TIME

std::mutex write_mtx;

FILE *fp;
static int SESSION_TRACKER; //Keeps track of session

#ifdef TIME
char* print_time() {
	int size = 0;
	time_t t;
	char *buf;

	t = time(NULL); //get current calendar time

	char *timestr = asctime(localtime(&t));
	timestr[strlen(timestr) - 1] = 0;  //Getting rid of \n

	size = strlen(timestr) + 1 + 2; //Additional +2 for square braces
	buf = (char*) malloc(size);

	memset(buf, 0x0, size);
	snprintf(buf, size, "[%s]", timestr);

	return buf;
}
#endif

void LOG(std::string fmts, ...) {
    char *fmt = (char *)fmts.c_str();
#ifdef DEBUG
	va_list list;
	char *p, *r;
	int e;
	write_mtx.lock();
	if (SESSION_TRACKER > 0)
		fp = fopen(LOGFILE, "a+");
	else
		fp = fopen(LOGFILE, "w");

#ifdef TIME
	fprintf(fp, "%s ", print_time());
	//fprintf(fp,"[%s][line: %d] ",line);
#endif

	va_start(list, fmts);
	for (p = fmt; *p; ++p) {
		if (*p != '%') //If simple string
				{
			fputc(*p, fp);
		} else {
			switch (*++p) {
			/* string */

			case 's': {
				r = va_arg(list, char *);

				fprintf(fp, "%s", r);
				continue;
			}

				/* integer */

			case 'd': {
				e = va_arg(list, int);

				fprintf(fp, "%d", e);
				continue;
			}

			default:
				fputc(*p, fp);
			}
		}
	}
	va_end(list);
	fputc('\n', fp);
	SESSION_TRACKER++;
	fclose(fp);
	write_mtx.unlock();
#else
	//fprintf(fp,"Not is DEBUG Mode!");
#endif
}

