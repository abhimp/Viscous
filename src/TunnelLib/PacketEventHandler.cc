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
 * EventHandler.cc
 *
 *  Created on: 23-Mar-2017
 *      Author: abhijit
 */

#include "PacketEventHandler.hh"


PacketEventHandler::PacketEventHandler(BaseReliableObj* parent) : BaseReliableObj(parent)
        , bigLoopTid(0)
        , evLoopTid(0)
        , conLoopTid(0)
        , timerLoopTid(0)
        , bigLoopThreadRunning(FALSE)
        , evLoopThreadRunning(FALSE)
        , conLoopThreadRunning(FALSE)
        , timerLoopThreadRunning(FALSE)
        , stopEvThread(FALSE)
        , con(NULL)
        , begQ(NULL)
        , endQ(NULL)
        , queueSize(0)
        , pendingTimeOut(FALSE)
        , closed(FALSE)
        , timerLoopThreadClosed(FALSE)
{
    con = new Connection(this);
}

PacketEventHandler::~PacketEventHandler() {
    if(!closed) close();
    delete con;
}

appSInt PacketEventHandler::recvPacketFrom(Packet* pkt, RecvSendFlags &flags) {
    RecvSendFlags flag = DEFALT_RECVSENDFLAG_VALUE;
    parent->recvPacketFrom(pkt, flag);
//    queueLock.lock();
//    addNextPacket(pkt);
//    notify.notify();
//    queueLock.unlock();
    return 0;
}

appInt PacketEventHandler::timeoutEvent(appTs time) {
    return 0;
}

appStatus PacketEventHandler::startClient(appInt localPort, appByte* localIp) {
    auto ret = con->startClient(localPort, localIp);
    start();
    return ret;
}

appStatus PacketEventHandler::startServer(appInt localPort, appByte* localIp) {
    auto ret = con->startServer(localPort, localIp);
    start();
    return ret;
}

void PacketEventHandler::close(void) {
//    closed = TRUE;
//    notify.notify();
//    waitToClose.wait();

    con->force_close();
    timerLoopThreadClosed = TRUE;
//    if(conLoopThreadRunning){pthread_cancel(conLoopTid);}
//    if(timerLoopThreadRunning){pthread_cancel(timerLoopTid);}
    if(conLoopThreadRunning){conLoopThreadClosedWait.wait();}
    if(timerLoopThreadRunning){timerLoopThreadClosedWait.wait();}
//    con->close();
}

void PacketEventHandler::start() {
    auto tmpO = this;
    appByte* data;
//    data = APP_PACK(tmpO);
//    if(runInThreadGetTid(PacketEventHandler::startBigLoopInsideThread, data, FALSE, &bigLoopTid) != APP_SUCCESS){
//        LOGE("Cannot create thread");
//        exit(__LINE__);
//    }
//    bigLoopThreadRunning = TRUE;
    data = APP_PACK(tmpO);
    if(runInThreadGetTid(PacketEventHandler::startConListnerInsideThread, data, FALSE, &conLoopTid) != APP_SUCCESS){
        LOGE("Cannot create thread");
        exit(__LINE__);
    }
    conLoopThreadRunning = TRUE;
    data = APP_PACK(tmpO);
    if(runInThreadGetTid(PacketEventHandler::startTimerInsideThread, data, FALSE, &timerLoopTid) != APP_SUCCESS){
        LOGE("Cannot create thread");
        exit(__LINE__);
    }
    timerLoopThreadRunning = TRUE;
}

//static
void* PacketEventHandler::startConListnerInsideThread(void* data,
        appThreadInfoId tid) {
    PacketEventHandler *peh;
    APP_UNPACK((appByte *)data, peh);
    peh->conLoopThreadRunning = TRUE;
    peh->con->listen();
    peh->conLoopThreadRunning = FALSE;
    peh->conLoopThreadClosedWait.notify();
    return NULL;
}

//static
void* PacketEventHandler::startTimerInsideThread(void* data,
        appThreadInfoId tid) {
    PacketEventHandler *peh;
    APP_UNPACK((appByte *)data, peh);
    peh->timerLoopThreadRunning = TRUE;
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = TIMER_GRANULITY_US*1000;
    while(1){
        clock_nanosleep(CLOCK_MONOTONIC, 0, &req, NULL);
        if(peh->closed or peh->timerLoopThreadClosed)
            break;
        peh->evTimerExpired();
    }
    peh->timerLoopThreadClosedWait.notify();
    peh->timerLoopThreadRunning = FALSE;
    return NULL;
}

//Static
//void* PacketEventHandler::startEventListnerInsideThread(void* data,
//        appThreadInfoId tid) {
//    PacketEventHandler *peh;
//    APP_UNPACK((appByte *)data, peh);
//    peh->evLoopThreadRunning = TRUE;
//    ev_run(EV_DEFAULT_ 0);
//    return NULL;
//}

//static
//void PacketEventHandler::evTimerExpired(EV_P_ ev_timer *w, int revents){
//    PacketEventHandler *peh = (PacketEventHandler *)w->data;
//    if(peh->stopEvThread){
//        ev_break(EV_DEFAULT_ EVBREAK_ALL);
//        ev_loop_destroy(EV_DEFAULT);
//        return;
//    }
//    peh->evTimerExpired();
//    ev_timer_again(EV_DEFAULT_ w);
//}

void PacketEventHandler::evTimerExpired() {
    parent->timeoutEvent(getTime());
}

//Static
void* PacketEventHandler::startBigLoopInsideThread(void* data,
        appThreadInfoId tid) {
    PacketEventHandler *self;
    APP_UNPACK((appByte *)data, self);
    self->bigLoop();
    return NULL;
}

void PacketEventHandler::bigLoop() {
    appBool performTimeOut = FALSE;
    Packet *pkt;
    while(1){
        notify.wait();
        if(pendingTimeOut){
            performTimeOut = TRUE;
            pendingTimeOut = FALSE;
        }
        else if(queueSize){
            pkt = getNextPacket();
        }
        queueLock.unlock();

        if(closed){
            waitToClose.notify();
            pthread_exit(NULL);
        }
        else if(performTimeOut){
            parent->timeoutEvent(getTime());
            performTimeOut = FALSE;
        }
        else{
            RecvSendFlags flag = DEFALT_RECVSENDFLAG_VALUE;
            parent->recvPacketFrom(pkt, flag);
        }
    }
}

Packet* PacketEventHandler::getNextPacket() {
    APP_ASSERT(begQ);
    readWriteLock.lock();
    auto tmp = begQ;
    begQ = (Packet *)begQ->next;
    if(begQ == NULL){
        endQ = NULL;
    }
    queueSize --;
    readWriteLock.unlock();
    tmp->next = NULL;
    return tmp;
}

void PacketEventHandler::addNextPacket(Packet* pkt) {
    APP_ASSERT(pkt);
    readWriteLock.lock();
    pkt->next = NULL;
    if(begQ != NULL){
        endQ->next = pkt;
        endQ = pkt;
    }
    else{
        begQ = endQ = pkt;
    }
    queueSize ++;
    readWriteLock.unlock();
}

void PacketEventHandler::waitToFinishAllThreads() {
    stopEvThread = TRUE;
    if(evLoopThreadRunning) pthread_join(evLoopTid, NULL);
    if(bigLoopThreadRunning) pthread_join(bigLoopTid, NULL);
    if(conLoopThreadRunning) pthread_join(conLoopTid, NULL);
    if(timerLoopThreadRunning) pthread_join(timerLoopTid, NULL);
}
