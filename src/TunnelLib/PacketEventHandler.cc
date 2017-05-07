/*
 * EventHandler.cc
 *
 *  Created on: 23-Mar-2017
 *      Author: abhijit
 */

#include "PacketEventHandler.h"


PacketEventHandler::PacketEventHandler(BaseReliableObj* parent)
    :BaseReliableObj(parent), bigLoopTid(0), evLoopTid(0), conLoopTid(0), timerLoopTid(0),
     bigLoopThreadRunning(FALSE), evLoopThreadRunning(FALSE), conLoopThreadRunning(FALSE), timerLoopThreadRunning(FALSE),
     stopEvThread(FALSE),
     con(NULL), begQ(NULL), endQ(NULL), queueSize(0), pendingTimeOut(FALSE), closed(FALSE)
{
    con = new Connection(this);
}

PacketEventHandler::~PacketEventHandler() {
    if(!closed) close();
    delete con;
}

appSInt PacketEventHandler::recvPacketFrom(Packet* pkt) {
    queueLock.lock();
    addNextPacket(pkt);
    notify.notify();
    queueLock.unlock();
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
    closed = TRUE;
    notify.notify();
    waitToClose.wait();
    if(conLoopThreadRunning){pthread_cancel(conLoopTid);}
    con->close();
}

void PacketEventHandler::start() {
    ev_init(&timer, evTimerExpired);
    timer.repeat = TIMER_GRANULITY_MS/1000.0; //200milisec
    timer.data = this;
    ev_timer_again(EV_DEFAULT_ &timer);
    auto tmpO = this;
    appByte* data;
    data = APP_PACK(tmpO);
    if(runInThreadGetTid(PacketEventHandler::startBigLoopInsideThread, data, FALSE, &bigLoopTid) != APP_SUCCESS){
        LOGE("Cannot create thread");
        exit(__LINE__);
    }
    bigLoopThreadRunning = TRUE;
//    data = APP_PACK(tmpO);
//    runInThreadGetTid(PacketEventHandler::startEventListnerInsideThread, data, FALSE, &evLoopTid);
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
        if(peh->closed)
            break;
        clock_nanosleep(CLOCK_MONOTONIC, 0, &req, NULL);
        peh->evTimerExpired();
    }
    return NULL;
}

//Static
void* PacketEventHandler::startEventListnerInsideThread(void* data,
        appThreadInfoId tid) {
    PacketEventHandler *peh;
    APP_UNPACK((appByte *)data, peh);
    peh->evLoopThreadRunning = TRUE;
	ev_run(EV_DEFAULT_ 0);
	return NULL;
}

//static
void PacketEventHandler::evTimerExpired(EV_P_ ev_timer *w, int revents){
    PacketEventHandler *peh = (PacketEventHandler *)w->data;
    if(peh->stopEvThread){
        ev_break(EV_DEFAULT_ EVBREAK_ALL);
        ev_loop_destroy(EV_DEFAULT);
        return;
    }
	peh->evTimerExpired();
	ev_timer_again(EV_DEFAULT_ w);
}

void PacketEventHandler::evTimerExpired() {
//    queueLock.lock();
    parent->timeoutEvent(getTime());
//    pendingTimeOut = TRUE;
//    notify.notify();
//    queueLock.unlock();
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
//    appSInt qSize;
    while(1){
//        queueLock.lock();
        notify.wait();
//        if(!pendingTimeOut and !closed and queueSize == 0){
////            queueLock.unlock();
////            notify.wait();
////            queueLock.lock();
//        }
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
            parent->recvPacketFrom(pkt);
        }
    }
}

Packet* PacketEventHandler::getNextPacket() {
    APP_ASSERT(begQ);
    readWriteLock.lock();
    auto tmp = begQ;
    begQ = begQ->next;
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
