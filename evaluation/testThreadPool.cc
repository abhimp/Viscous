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
 * testThreadPool.cc
 *
 *  Created on: 29-Mar-2017
 *      Author: abhijit
 */

#include <iostream>
#include <unistd.h>
#include <common.h>
#include "../src/util/ThreadPool.hh"

util::AppMutex fprintlock;
void f1(int n, int id)
{
    for (int i = 0; i < 5; ++i) {
        fprintlock.lock();
        std::cout << "fn:1 n:" << n <<" id:" << id << " i:" << i << " executing\n";
        fprintlock.unlock();
//        ++n;
        sleep(n);
    }
}

void f2(int n, int id)
{
    for (int i = 0; i < 5; ++i) {
        fprintlock.lock();
        std::cout << "fn:2 n:" << n <<"id:" << id << " i:" << i << " executing\n";
//        ++n;
        fprintlock.unlock();
        sleep(1);
    }
}

void *runinthreadtest(void *data){
    int n;
    int id;
    int fn;
    APP_UNPACK((appByte*) data, fn, n, id);
    if(fn == 1){
        f1(n, id);
    }
    else{
        f2(n, id);
    }
    return NULL;
}

int threadTest()
{
    util::ThreadPool p(15);
    p.run();
    int fn, n;
    for(auto i = 0; i < 15; i++){
        fn = i%2 + 1;
        n = std::rand()%6;
        appByte *data = APP_PACK(fn, n, i);
        p.executeInsidePool(runinthreadtest, data);
    }
//    sleep(50);
//    p.stop();
    return 0;
}
