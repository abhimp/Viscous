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
 * NewFlowHandler.cc
 *
 *  Created on: 10-Aug-2017
 *      Author: abhijit
 */

#include "NewFlowHandler.hh"

#include <cstring>

#include "../Packet.h"
#include "../PacketPool.hh"

namespace FlowHandler {

NewFlowHandler::~NewFlowHandler() {
    LOGI("destructor at NewFlowHandler for flow: %d, fingerprint: %d", flowId_.flowId, flowId_.fingerPrint);
    if(rcvPktQueue){
        delete[] rcvPktQueue;
        rcvPktQueue = NULL;
    }
}

NewFlowHandler::NewFlowHandler(BaseReliableObj* parent, appFlowIdType flowId, appInt16 rcvWnd) : ReliabilityMod(parent, flowId)
            , rcvPktQueue(NULL)
            , readUpto(APP_INT16_MAX)
            , availableUpTo(APP_INT16_MAX)
            , closeInitiated(FALSE)
            , closed(FALSE)
            , startedRecv(FALSE)
            , startedSend(FALSE)
            , remoteFreeWindow(rcvWnd)
            , ackedBitField(APP_INT16_CNT)
            , ackedUpto(APP_INT16_MAX)
            , nextSeqNumber(APP_INT16_MAX)
            , recvWindow(rcvWnd)
{
    LOGI("Creating NewFlowHandler for flow: %d, fingerprint: %d", flowId_.flowId, flowId_.fingerPrint);
}

appSInt NewFlowHandler::recvPacketFrom(Packet* pkt, RecvSendFlags& flags) {
    LOGI("pkt recv: selfFlowId: %d, flowId: %d, seqno: %d", flowId_.flowId, pkt->header.flowId, pkt->header.seqNo);
    APP_ASSERT(pkt);
    if((pkt->header.flag&FLAG_DAT) and !rcvPktQueue){
        rcvPktQueue = new Packet*[APP_INT16_CNT];
        startedRecv = TRUE;
        memset(rcvPktQueue, 0, APP_INT16_CNT * sizeof(Packet*));
    }

    if(!(pkt->header.flag&FLAG_DAT)){
        return -ERROR_INVALID_PACKET;
    }

    appInt16 virSeqNo = pkt->header.flowSeqNo - (readUpto + 1);
    appInt16 seqNo = pkt->header.flowSeqNo;
    if(virSeqNo > ((APP_INT16_CNT>>2) + (APP_INT16_CNT>>1))){ //checking if duplicate packet or not.
        return -ERROR_DUPLICATE_PACKET;
    }
    APP_ASSERT(virSeqNo <= APP_INT16_CNT/2);

    if(rcvPktQueue[seqNo]){
        LOGI("Duplicate Packet flowId: %d, seq: %d", flowId_.flowId, seqNo);
        return -ERROR_DUPLICATE_PACKET;
    }

    readLock.lock();
    rcvPktQueue[seqNo] = pkt;
    appBool closeNotify = FALSE;
    for(appInt16 nextAvailable = availableUpTo + 1; rcvPktQueue[nextAvailable]; nextAvailable++){
        availableUpTo++;
        auto npkt = rcvPktQueue[availableUpTo];
        if(npkt->header.flag&FLAG_FFN){
            if(closeInitiated){ // if close initiated by this end then this packet is the acknowledgment to the close request.
                closed = TRUE;
                closeNotify = TRUE;
            } else{
                sendClosePacket();
                closed = TRUE;
            }
            readSem.notify();
            break;
        }
        readSem.notify();
    }
    readLock.unlock();
    if(closeNotify)
        closeSemaphore.notify();
    return 0;
}

appSInt NewFlowHandler::readData(appFlowIdType flowId, appByte* data,
        appInt size) {
    if(!data or !size or size < MAX_PACKET_SENT_DATA_SIZE){
        return -ERROR_INVALID_READ_SIZE;
    }
    readSem.wait();
    APP_ASSERT(readUpto != availableUpTo);
    readLock.lock();
    appInt16 nextPkt = readUpto + 1;
    APP_ASSERT(rcvPktQueue[nextPkt]);
    if(closeInitiated and closed){ //this will occures only the user closes a flow from different thread than it reads.
        readLock.unlock();
        return 0;
    }

    auto pkt = rcvPktQueue[nextPkt];
    readUpto++;
    rcvPktQueue[readUpto] = NULL;
    readLock.unlock();
    if(pkt->header.flag & FLAG_FFN){
        getPacketPool().freePacket(pkt);
        return 0;
    }
    if(pkt->len == 0){
        return -ERROR_NO_DATA;
    }
    memcpy(data, pkt->data, pkt->len);
    auto len = pkt->len;
    getPacketPool().freePacket(pkt);
    return len;
}

appSInt NewFlowHandler::sendPacketTo(appInt id, Packet* pkt) {

    remoteFreeWindow.wait();
    ackedBitField.resetBit(pkt->header.flowSeqNo);
    pkt->header.flowId = flowId_.flowId;
    auto ret = parent->sendPacketTo(id, pkt);
    APP_ASSERT(ret == 0);
    LOGI("Sending data to other side: seq: %d, flowseq: %d, flowid: %d", pkt->header.seqNo, pkt->header.flowSeqNo, flowId_.flowId);
    return 0;
}

appSInt NewFlowHandler::sendData(appFlowIdType flowId, appByte* data,
        appInt dataLen) {
    appSInt count = 0;
    while(dataLen >= MAX_PACKET_SENT_DATA_SIZE){
        Packet *pkt = getPacketPool().getNewPacketWithData();
        memcpy(pkt->data, data, MAX_PACKET_SENT_DATA_SIZE);
        data += MAX_PACKET_SENT_DATA_SIZE;
        dataLen -= MAX_PACKET_SENT_DATA_SIZE;
        pkt->len = MAX_PACKET_SENT_DATA_SIZE;
        pkt->header.flowSeqNo = getNextSeqNo();
        if(sendPacketToCloseCheck(id, pkt)){
            return count;
        }
        count += MAX_PACKET_DATA_SIZE;
    }
    if(dataLen <= MAX_PACKET_SENT_DATA_SIZE && dataLen > 0){
        Packet *pkt = getPacketPool().getNewPacketWithData();
        memcpy(pkt->data, data, dataLen);
        pkt->len = dataLen;
        pkt->header.flowSeqNo = getNextSeqNo();
//        sendPacketTo(id, pkt, NULL, 0);
        if(sendPacketToCloseCheck(id, pkt)){
            return count;
        }
        count += dataLen;
    }
    return count;
}

appInt16 NewFlowHandler::getNextSeqNo() {
    nextSeqNumber ++;
    return nextSeqNumber;
}

appInt16 NewFlowHandler::getReadUpto() {
    return readUpto;
}

appStatus NewFlowHandler::closeFlow(appFlowIdType flowId) {
    if(closed or closeInitiated)
        return APP_FAILURE;
    closeLock.lock();
    if(closed or closeInitiated){
        closeLock.unlock();
        return APP_SUCCESS;
    }
    sendClosePacket();
    closeInitiated = TRUE;
    closeLock.unlock();
    closeSemaphore.wait();
    return APP_SUCCESS;
}

void NewFlowHandler::sendClosePacket() {
    Packet *npkt = getPacketPool().getNewPacket(TRUE);
    npkt->reInitHeader();
    npkt->header.flowSeqNo = getNextSeqNo();
    npkt->header.flag |= FLAG_FFN;
    sendPacketTo(id, npkt);
}

appInt NewFlowHandler::timeoutEvent(appTs time) {
    APP_ASSERT(0 && "NOT ALLOWED");
    return 0;
}

inline appSInt NewFlowHandler::recvAck(appFlowIdType flowId,
        appInt16 flowSeqNo) {
    appInt16 read = flowSeqNo - ackedUpto;
    if(read > recvWindow)
        return 0;
    ackedUpto = flowSeqNo;
    for(auto x = read; x; x--)
        remoteFreeWindow.notify();
    return 0;
}

appStatus NewFlowHandler::recvAck(PacketAckHeader* pktAckHdr) {
    appInt16 flowSeqNo = pktAckHdr->ackNo;
    appInt16 read = flowSeqNo - ackedUpto;
    ackedUpto = flowSeqNo;
    for(auto x = read; x; x--)
        remoteFreeWindow.notify();
    return APP_SUCCESS;
}

void NewFlowHandler::initiateClosure() {
    APP_ASSERT(0 && "NOT ALLOWED");
}

void NewFlowHandler::setCallBack(EventType evt, void* dataToPass, void* func) {
    APP_ASSERT(0 && "NOT ALLOWED");
}

} /* namespace FlowHandler */

