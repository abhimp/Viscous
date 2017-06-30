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
 * SendingSocket.cpp
 *
 *  Created on: 05-Apr-2017
 *      Author: abhijit
 */

#include "SendingSocket.h"
#include <iostream>
#include <common.h>
#include "../CommonHeaders.hh"

namespace Interface {

static struct timespec timeOuts[] = {
        {.tv_sec = 0, .tv_nsec =   50},
        {.tv_sec = 0, .tv_nsec =  120},
        {.tv_sec = 0, .tv_nsec =  250},
        {.tv_sec = 0, .tv_nsec =  500},
        {.tv_sec = 0, .tv_nsec = 1000},
};

static int backOffCount = sizeof(timeOuts)/sizeof(struct timespec);


AppPool<SendMsgBuffer>& getSendMsgPool() {
    static AppPool<SendMsgBuffer> sendMsgPool;
    return sendMsgPool;
}

SendingSocket *SendingSocket::instance = NULL;

std::mutex SendingSocket::instantCreationMutex;

SendingSocket::SendingSocket() :
    socket_descriptor(0),
    begQ(NULL),
    endQ(NULL),
    sizeQ(0),
    sendingThreadRunning(FALSE),
    stopSendingThread(FALSE),
    sendingTid(0),
    packetCount(0),
    byteCount(0L)
{
    init();
}

SendingSocket* SendingSocket::getInstance() {
    if(instance){
        return instance;
    }
    instantCreationMutex.lock();
    if(instance){
        instantCreationMutex.unlock();
        return instance;
    }
    instance = new SendingSocket();
    instantCreationMutex.unlock();
    return instance;
}

int SendingSocket::init() {
    if ((socket_descriptor = socket (PF_PACKET, SOCK_DGRAM, htons (ETH_P_ALL))) < 0) {
		perror ("socket() failed ");
		return 3;
	}
    auto tmpO = this;
    auto data = APP_PACK(tmpO);
    runInThreadGetTid(SendingSocket::startSendingInsideThread, data, FALSE, &sendingTid);
    return 0;
}

SendMsgBuffer* SendingSocket::getNextPacket() {
    if(!begQ)
        return NULL;
    auto tmp = begQ;
    if(begQ == endQ){
        endQ = endQ->next;
    }
    begQ = begQ->next;
    sizeQ --;
    return tmp;
}

void SendingSocket::addNextPacket(SendMsgBuffer* pkt) {
    pkt->next = NULL;
    if(begQ){
        endQ->next = pkt;
        endQ = pkt;
    }
    else{
        begQ = endQ = pkt;
    }
    sizeQ ++;
}

void* SendingSocket::startSendingInsideThread(void* data, appThreadInfoId tid) {
    SendingSocket *obj;
    APP_UNPACK((appByte*)data, obj);
    obj->sendPacketFromQueue();
    return NULL;
}

int SendingSocket::sendMsgToSystemInterface(SendMsgBuffer *msg) {
    int ret = 0;
    int retryCnt = 0;
    do{
        ret = sendto (socket_descriptor, msg->ether_frame, msg->frame_len, 0, (struct sockaddr *) &msg->device, msg->deviceLen);
        if(ret >= 0){
            break;
        }
        else if(errno != ENOBUFS){
            std::cerr << "Error " << retryCnt << " error No: " << errno <<  std::endl;
            perror("error");
            break;
        }
        else{
            if(retryCnt > 0)
                std::cout << "Error: " << retryCnt << std::endl;
            auto ptr = retryCnt < backOffCount ? &timeOuts[retryCnt] : &timeOuts[backOffCount-1];
            clock_nanosleep(CLOCK_MONOTONIC, 0, ptr, NULL);
            retryCnt ++;
        }
    }while(retryCnt < 10);
    packetCount += 1;
    byteCount += msg->frame_len;
    LOGI("SenderInterfacePacketCount: %ld, %u, %lu", appTime.getTime().getMicro(), packetCount, byteCount);
    return ret;
}

SendingSocket::~SendingSocket() {
}

void SendingSocket::sendPacketFromQueue() {
    sendingThreadRunning = TRUE;
    while(!stopSendingThread){
        waitForPacket.wait();
        packeQueueLock.lock();
        auto msg = getNextPacket();
        packeQueueLock.unlock();
        if(msg == NULL){
            continue;
        }

        sendMsgToSystemInterface(msg);
        getSendMsgPool().free(msg);
    }
}

void SendingSocket::sendMsg(SendMsgBuffer* msg) {

    packeQueueLock.lock();
    addNextPacket(msg);
    packeQueueLock.unlock();
    waitForPacket.notify();
}

} /* namespace Interface */
