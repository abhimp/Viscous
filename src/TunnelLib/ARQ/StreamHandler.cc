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
 * StreamHandler.cpp
 *
 *  Created on: 12-Dec-2016
 *      Author: abhijit
 */

#include "StreamHandler.hpp"
#include "../PacketPool.h"
#include "../../util/ThreadPool.h"

//#define UNLIMITED_MEMORY
#ifndef UNLIMITED_MEMORY
#define RECV_BUFFER 5000
#endif
#define APP_MAX(x,y) ((x) > (y) ? (x) : (y))
#define APP_ABS(x) ((x) < 0 ? (-1)*(x) : (x))


StreamHandler::StreamHandler(BaseReliableObj *parent, appInt16 flowId) :
    ReliabilityMod(parent, flowId), recvBuffer(SIZE_8MB), sendBuffer(1300), ackedBitField(APP_INT16_CNT),
    nextSeqNumber(-1), recvQueue(NULL), firstPacketReceived(FALSE), recvdPktUpto(APP_INT16_MAX),
    readPacketUpto(APP_INT16_MAX), ackedPacketUpto(APP_INT16_MAX),
    recvAckedPacketOffset(0),
//    ackedBuffer(APP_INT16_CNT),
    connectionClosed(FALSE), closingInitiated(FALSE), packetsPendingToRead(0), totalPacketsRecvd(0), jobsInThread(0),
    newDataCB(NULL), closingFlowCB(NULL), newDataCBInfo(NULL), closingFlowCBInfo(NULL), worker(NULL)
{

}

StreamHandler::~StreamHandler() {
    closeFlow(flowId_);
    if(recvQueue){
        delete[] recvQueue;
    }
    LOGI("destroying: buffer size %ld", recvBuffer.size());
}

appStatus StreamHandler::closeFlow(appInt16 flowId)
{
    closingMutex.lock();
    if(connectionClosed){
        closingMutex.unlock();
        return APP_SUCCESS;
    }
    LOGI("Closing flowing: flowid: %d", flowId_);
    sendClosePacket();
    closingInitiated = TRUE;
    LOGI("waiting to close: flowid: %d", flowId_);
    waitForCloseReport.wait();
    LOGI("flow closed: flowid: %d", flowId_);
//    parent->closeFlow(flowId);
    closingMutex.unlock();
    APP_RETURN_SUCCESS
}

appSInt StreamHandler::sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen)
{
    appInt16 virSeqNumber;
    virSeqNumber = APP_POSITIVE_MOD(pkt->header.flowSeqNo - recvAckedPacketOffset, APP_INT16_CNT);
    if(virSeqNumber >= RECV_BUFFER){
        waitForSenderSlot.wait();
    }
    ackedBitField.resetBit(pkt->header.flowSeqNo);
    pkt->header.flowId = flowId_;
    auto ret = parent->sendPacketTo(id, pkt, dest_addr, addrlen);
    APP_ASSERT(ret == 0);
    LOGI("Sending data to other side: seq: %d, flowseq: %d, flowid: %d", pkt->header.seqNo, pkt->header.flowSeqNo, flowId_);
    return 0;
}

appStatus StreamHandler::recvAck(PacketAckHeader *pktAckHdr){
    APP_ASSERT(pktAckHdr);
    appInt16 virPktAckNo = APP_POSITIVE_MOD(pktAckHdr->ackNo - recvAckedPacketOffset, APP_INT16_CNT);
    if(virPktAckNo >= APP_INT16_CNT/2)
        APP_RETURN_SUCCESS;
//    ackedBuffer.setBit(pktAckHdr->ackNo);
//    while(ackedBuffer[recvAckedPacketOffset]){
    recvAckedPacketOffset = pktAckHdr->ackNo+1;
    waitForSenderSlot.notify();
//    }
    APP_RETURN_SUCCESS;
}

appSInt StreamHandler::recvPacketFrom(Packet *pkt)
{
    if(!recvQueue){
        recvQueue = new Packet*[APP_INT16_CNT];
        memset(recvQueue, 0, sizeof(Packet *)*APP_INT16_CNT);
    }
    if(connectionClosed) return -1;

    if(pkt->header.flag&FLAG_DAT){
        appInt16 virSeqNo = pkt->header.flowSeqNo - readPacketUpto;
        if(virSeqNo > APP_INT16_CNT/2){
            LOGI("Ignoring packet: flowSeq: %d, readPacketUpto: %d", pkt->header.flowSeqNo, readPacketUpto);
            return -__LINE__;
        }
        if(recvQueue[pkt->header.flowSeqNo]){
            LOGI("Packet already exists: flowSeq: %d, readPacketUpto: %d", pkt->header.flowSeqNo, readPacketUpto);
            return -__LINE__; //We have recved this Packet before.
        }
        if(pkt->header.flag&FLAG_FFN) LOGI("Recved FFN pkt: seq: %d, flowSeq: %d, flowId: %d", pkt->header.seqNo, pkt->header.flowSeqNo, pkt->header.flowId);
        recvQueue[pkt->header.flowSeqNo] = pkt;
        readPacketLock.lock();
        packetsPendingToRead ++;
        readPacketLock.unlock();
//        waitToRead.notify();
        appInt16 nextPkt = readPacketUpto + 1;
        auto self = this;
        for(; recvQueue[nextPkt] and newDataCB and newDataCBInfo; nextPkt++){
            readPacketUpto = nextPkt;
//            appInt16 nextPkt;
            auto pkt = recvQueue[nextPkt];
            recvQueue[nextPkt] = NULL;
            if(pkt->header.flag&FLAG_FFN){
                self->finishClosing();
                getPacketPool().freePacket(pkt);
                break;
            }
            newDataCB(newDataCBInfo, pkt->data, pkt->len);
//            auto data = APP_PACK(self, pkt);
//            runInThreadPool(StreamHandler::sendRecvdPacket, data);
        }
        return 0;
    }
    return -2;
}

//appSInt StreamHandler::readData(appInt16 flowId, appByte *data, appInt size){
//    auto ret = recvBuffer.read(data, size);
////    if(ret == 0 && recvBuffer.closed()){
////        finishClosing();
////    }
//    return ret;
//}

appSInt StreamHandler::sendData(appInt16 flowId, appByte *data, appInt dataLen)
{
//#ifdef UNLIMITED_MEMORY
//    LOGI("SENDDATA: sending %d bytes", dataLen);
    while(dataLen >= MAX_PACKET_SENT_DATA_SIZE){
        Packet *pkt = getPacketPool().getNewPacketWithData();
        std::memcpy(pkt->data, data, MAX_PACKET_SENT_DATA_SIZE);
        data += MAX_PACKET_SENT_DATA_SIZE;
        dataLen -= MAX_PACKET_SENT_DATA_SIZE;
        pkt->len = MAX_PACKET_SENT_DATA_SIZE;
        pkt->header.flowSeqNo = getNextSeqNo();
        sendPacketTo(id, pkt, NULL, 0);
    }
    if(dataLen <= MAX_PACKET_SENT_DATA_SIZE && dataLen > 0){
        Packet *pkt = getPacketPool().getNewPacketWithData();
        std::memcpy(pkt->data, data, dataLen);
        pkt->len = dataLen;
        pkt->header.flowSeqNo = getNextSeqNo();
        sendPacketTo(id, pkt, NULL, 0);
    }
    return 0;
//#endif //UNLIMITED_MEMORY
}

appInt StreamHandler::timeoutEvent(appTs time)
{
    return 0;
}

void StreamHandler::printStat()
{
}

appInt16 StreamHandler::getNextSeqNo(){
    nextSeqNumber ++;
    return nextSeqNumber;
}

void StreamHandler::sendClosePacket() {
    Packet *npkt = getPacketPool().getNewPacket(TRUE);
    npkt->reInitHeader();
    npkt->header.flowSeqNo = getNextSeqNo();
    npkt->header.flag |= FLAG_FFN;
    sendPacketTo(id, npkt, NULL, 0);
}

void StreamHandler::sendAck(appInt16 ackNo) {
    PacketAckHeader *pktAckHdr = GET_OPTIONAL_PACKET_HEADER_TYPE_ACK_HEADER;
    pktAckHdr->ackNo = ackNo;
    pktAckHdr->flowId = flowId();
    pendingAcks().addAck(pktAckHdr);
}

appSInt StreamHandler::recvAck(appInt16 flowId, appInt16 flowSeqNo) {
    appInt16 virPktAckNo = APP_POSITIVE_MOD(flowSeqNo - recvAckedPacketOffset, APP_INT16_CNT);
    if(virPktAckNo >= APP_INT16_CNT/2)
        return -1;
    ackedBitField.setBit(flowSeqNo);
    appInt16 cnt = 0;
//    appInt16 nextToRead =
    for(;ackedBitField[recvAckedPacketOffset];recvAckedPacketOffset++, cnt++){
        ackedBitField.resetBit(recvAckedPacketOffset);
    }
    if(cnt)
        waitForSenderSlot.notify();

    return 0;
}

void StreamHandler::finishClosing() {
    if(closingInitiated and !connectionClosed){
        connectionClosed = TRUE;
        if(closingFlowCB) closingFlowCB(closingFlowCBInfo);
        waitForCloseReport.notify();
    }
    else{
//        sendClosePacket();
        connectionClosed = TRUE;
        runInThreadPool(StreamHandler::sendClosePacketInsideThread, this);
        if(closingFlowCB) closingFlowCB(closingFlowCBInfo);
    }
}

void StreamHandler::setCallBack(EventType evt, void *info, void* func) {
    switch (evt) {
        case EVENT_CLOSING:
//            closingFlowCB = reinterpret_cast<closingFlow>(func);
            closingFlowCB = (closingFlow)func;
            closingFlowCBInfo = info;
            break;
        case EVENT_NEW_DATA:
//            newDataCB = reinterpret_cast<newDataToFlow>(func);
            newDataCB = (newDataToFlow)func;
            newDataCBInfo = info;
            break;
        default:
            break;
    }
}

//static
void* StreamHandler::sendClosePacketInsideThread(void* data) {
    StreamHandler *self = (StreamHandler *)data;
    self->jobInThreadLock.lock();
    self->jobsInThread --;
    self->jobInThreadLock.unlock();
    self->sendClosePacket();
    return NULL;
}

//static
void* StreamHandler::sendRecvdPacket(void* data) {
    Packet *pkt;
    StreamHandler *self;
    APP_UNPACK((appByte *)data, self, pkt);

    self->jobInThreadLock.lock();
    self->jobsInThread --;
    self->jobInThreadLock.unlock();

    if(self->newDataCB) self->newDataCB(self->newDataCBInfo, pkt->data, pkt->len);
    getPacketPool().freePacket(pkt);
    return NULL;
}

void StreamHandler::initiateClosure() {
    if(connectionClosed)
        return;
    sendClosePacket();
}

void StreamHandler::runInThreadPool(UTIL::cb_func fn, void* data) {
    if(!worker) worker = getWorker(STREAM_COMMON_WORKER);
    jobInThreadLock.lock();
    jobsInThread ++;
    jobInThreadLock.unlock();
    worker->executeInsidePool(fn, data);
}
