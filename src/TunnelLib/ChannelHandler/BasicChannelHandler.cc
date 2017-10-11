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
 * BasicChannelHandler.cpp
 *
 *  Created on: 26-Dec-2016
 *      Author: abhijit
 */

#include "BasicChannelHandler.hh"
#include "../PacketPool.hh"

namespace channelHandler{

BasicChannelHandler::BasicChannelHandler(BaseReliableObj *parent,
        appInt8 ifcLoc, RemoteAddr &rmadr, appInt8 ifcSrc, appInt8 ifcDst, bool lock) :
        ChannelHandler(parent, ifcLoc, rmadr, ifcSrc, ifcDst)
        , senderWindow(1)
        , cwnd(1.)
        , duplicateCount(0)
        , ssthresh(INITIAL_SSTHRESH)
        , recover(0)
        , rtt(0)
        , srtt(0)
        , rto(TIMER_SECOND_IN_US)
        , rttvar(0)
        , G(TIMER_GRANULITY_US)
        , alpha(0.125)
        , beta(0.25)
        , packetLock(lock)
        , status(SLOW_START)
        , closed(FALSE)
        , deadChannel(FALSE)
        , started(FALSE)
{
}

void BasicChannelHandler::startUp() {
    sendCSNPacket();
}

BasicChannelHandler::~BasicChannelHandler() {
}


appInt16 BasicChannelHandler::getSenderWindowSize(){
    return (appInt16)cwnd;
}

appSInt BasicChannelHandler::sendPacket(Packet *pkt){
    packetLock.lock();
    if(closed){
        packetLock.unlock();
        return -9;
    }
    if(deadChannel){
        packetLock.unlock();
        return -6;
    }
    if(!started and (pkt->header.flag|FLAG_CSN) == 0){
        packetLock.unlock();
        return -3;
    }
    if(senderBuffer.lastAcked == senderBuffer.maxPacket) //i.e. there are no pending packet to send or acked
    {
        pkt->header.flag |= FLAG_CHS;
        senderBuffer.started = TRUE;
        addBuffer(pkt);
        sendNew(1);
    }
    else
    {
        appInt16 x = senderBuffer.maxPacket - senderBuffer.lastAcked;
        if(x >= (APP_INT16_CNT/2)){
            LOGI("No more space left: cwnd:%d", int(cwnd));
            packetLock.unlock();
            return -1;
        }
        addBuffer(pkt);
    }
    packetLock.unlock();
    return 0;
}

appInt BasicChannelHandler::timeoutEvent(appTs time){
    packetLock.lock();
    if(closed or deadChannel){
        packetLock.unlock();
        return 0;
    }
    appBool timeoutOccured = FALSE;
    appSInt64 time_ms = time.getMicro();
    appInt cnt = 0;
    auto tmp = senderBuffer.timerList.begin();
    while(tmp!=senderBuffer.timerList.end())
    {
        auto buf = *tmp;
        auto expectedSeq = buf->pkt_->header.seqNo;
        auto lastsend = buf->time_;
        auto timeout = buf->effTimer();
        cnt ++;
        if(lastsend != 0 and timeout <= time_ms)// and (cnt <= cwnd/2 or cnt <= 1))
        {
            if(expectedSeq == 0 and senderBuffer[expectedSeq].retryCnt_ > 2){
                LOGI("Channel dead: %d", id());
                deadChannel = TRUE;
//                APP_ASSERT(0);
                auto worker =  parent->getWorker(BaseReliableObj::STREAM_COMMON_WORKER);
                worker->executeInsidePool(forwardPacketsToIfcScheduler, this);
                break;
            }
            senderBuffer.stopTimer(expectedSeq);
            LOGI("timeout %d: %d, %ld, %ld,  %ld", cnt, expectedSeq, time_ms, timeout, time_ms - timeout);
            sendSeq(expectedSeq);
            timeoutOccured = TRUE;
            senderBuffer[expectedSeq].retryCnt_ ++;
            senderBuffer.totalRetransmitions ++;
            senderBuffer.totalTimeouts ++;
        }
        else
            break;
        tmp = senderBuffer.timerList.begin();
    }
    if(timeoutOccured){
        ssthresh = cwnd/2;
        if(ssthresh < 2) ssthresh = 2;
        updateCwnd(SET_TO_ONE);
        LOGI("Retransmission count: %ld, %d, %lu, %lu, %lu", time.getMicro(), id(), senderBuffer.totalDuplicateAck, senderBuffer.totalTimeouts, senderBuffer.totalRetransmitions);
    }
    packetLock.unlock();
    return 0;
}

appSInt BasicChannelHandler::recv(Packet *pkt){
    packetLock.lock();
    if(pkt->header.window != 0){
        senderWindow = pkt->header.window;
    }
    started = TRUE;
    if(pkt->header.flag & FLAG_ACK){
        ackRecv(pkt);
    }
//    if((pkt->len > 0 and pkt->data != NULL) or (pkt->header.flag&FLAG_FFN) or (pkt->header.flag&FLAG_FFN)){
    if((pkt->header.flag&FLAG_DAT)){
        pktRecv(pkt);
    }
    packetLock.unlock();
    return 0;
}

appInt BasicChannelHandler::pktRecv(Packet* pkt) {
    if(!recieverBuff.recvStarted and !(pkt->header.flag&FLAG_CHS))
        return 0;
    if(pkt->header.flag&FLAG_CHS)
        recieverBuff.recvStarted = TRUE;
    appInt16 normalizedSeqNo = pkt->header.seqNo - recieverBuff.expectedSeq;
    appInt16 normalizedMaxRecv = recieverBuff.maxRecvd - recieverBuff.expectedSeq;
    LOGI("pkt rcvd: %ld, %d, %d, %d, %d", getTime().getMicro(), recieverBuff.expectedSeq-1, pkt->header.seqNo, normalizedSeqNo, id());
    appInt16 expSeq = recieverBuff.expectedSeq;
    if(normalizedSeqNo >= 0 and normalizedSeqNo < (APP_INT16_CNT/2))
    {
        recieverBuff.buf.setBit(pkt->header.seqNo);
        for(; recieverBuff.buf.getBit(expSeq); expSeq++){
            recieverBuff.buf.resetBit(expSeq);
        }
        recieverBuff.expectedSeq = expSeq;
        sendAck(pkt, expSeq-1);
        if(normalizedSeqNo > normalizedMaxRecv)
            recieverBuff.maxRecvd = pkt->header.seqNo;

        pkt->accepted = TRUE;
    }
    else if(normalizedSeqNo > ((APP_INT16_CNT >> 1) + (APP_INT16_CNT >> 2))){ // check it is in forth quater
        sendAck(pkt, expSeq-1);
    }
    return 0;
}

void BasicChannelHandler::validatePacket(Packet* pkt) {
    appInt16 normalizedSeqNo = pkt->header.seqNo - recieverBuff.expectedSeq;
    if((pkt->header.flag&FLAG_DAT) and normalizedSeqNo >= 0 and normalizedSeqNo < (APP_INT16_CNT/2))
        pkt->accepted = TRUE;
}

appInt BasicChannelHandler::ackRecv(Packet* pkt) {
    appInt16 normalisedackno = pkt->header.ackNo - senderBuffer.lastAcked;
    if(!senderBuffer.started)
        return 0;
    appBool newAck = FALSE;
    appBool partialAck = FALSE;
    appBool duplicateAck = FALSE;
    auto time = getTime();
    if(normalisedackno > 0 and normalisedackno <= APP_INT16_CNT/2) // received expected ack
    {
        if(status == FAST_RECOVERY and normalisedackno < recover){
            partialAck = TRUE;
        }
        else{
            newAck = TRUE;
            status = SLOW_START;
            recover = 0;
        }
    }
    else if(normalisedackno == 0){
        if(senderBuffer.lastAcked != senderBuffer.maxSent)
            duplicateAck = TRUE;
    }
    else{
        return -1;
    }

    senderBuffer.totalSuccessfullySent += normalisedackno;

    if(newAck or partialAck) // received expected ack
    {
        appInt16 acked = senderBuffer.lastAcked + 1;
        senderBuffer.lastAcked = pkt->header.ackNo;
        waitToEmptySapce.notify();
        sendFreeCellNotification();
        updateRtt(pkt);
        duplicateCount = 0;
        LOGI("newackrecved trace: %ld, %d, %lu, %lu", time.getMicro(), id(), senderBuffer.totalSuccessfullySent, senderBuffer.totalSent);

        if(newAck){
            LOGI("ackrecvd: %d, sendpacket: %d, origack: %d", pkt->header.ackNo, normalisedackno+1, pkt->header.origAckNo);
        }
        else{
            LOGI("ackrecvd-partial: %d, sendpacket: %d,recover %d, origack: %d", pkt->header.ackNo, normalisedackno+1, recover, pkt->header.origAckNo);
        }
        for(auto x = 0; x < normalisedackno; x++){
            appInt16 tmpy = acked+x;
            senderBuffer.stopTimer(tmpy);
            freePacket(senderBuffer.buf[tmpy].pkt_);
            senderBuffer.buf[tmpy].reset();
        }
        if(newAck){
            if(cwnd <= ssthresh){
                updateCwnd(MULTIPLICATIVE_INCREASE);
                status = SLOW_START;
            }
            else{
                updateCwnd(ADDITIVE_INCREASE);
                status = CONG_AVOIDANCE;
            }
            sendNew(normalisedackno << 1);
        }
        else{
            recover = recover - normalisedackno;
            acked = senderBuffer.lastAcked + 1;
            appSInt64 time_ = time.getMicro();
            for(auto x = 0; x < recover; x++){
                appInt16 tmpy = acked + x;
                senderBuffer.restartTimer(tmpy, time_);
            }
            sendNew(1);
//            senderBuffer.stopTimer(acked);
//            sendSeq(acked);
//            duplicateCount = 3;
            updateCwnd(SET_TO, cwnd - normalisedackno + 1);
//            senderBuffer.totalDuplicateAck ++;
//            senderBuffer.totalRetransmitions ++;
//            LOGI("Retransmission count: %ld, %d, %lu, %lu, %lu", time.getMicro(), id(), senderBuffer.totalDuplicateAck, senderBuffer.totalTimeouts, senderBuffer.totalRetransmitions);
        }
    }
    else if(duplicateAck){
        duplicateCount += 1;
        if(duplicateCount == 3){
            appInt16 nextPktSeq = senderBuffer.lastAcked + 1;
            senderBuffer.stopTimer(nextPktSeq);
            sendSeq(nextPktSeq);
            if(status != FAST_RECOVERY){
                recover = senderBuffer.maxSent - senderBuffer.lastAcked;
                appInt16 flightSize = senderBuffer.maxSent - senderBuffer.lastAcked;
                ssthresh = APP_MAX(flightSize/2, 2); //rfc2581 eq 3
                updateCwnd(SET_TO, ssthresh + 3);
                if(status == CONG_AVOIDANCE or status == SLOW_START)
                    status = FAST_RECOVERY;
            }
            LOGI("Triple duplicate: %d, origack: %d", pkt->header.ackNo, pkt->header.origAckNo);
            senderBuffer.totalDuplicateAck ++;
            senderBuffer.totalRetransmitions ++;
            LOGI("Retransmission count: %ld, %d, %lu, %lu, %lu", time.getMicro(), id(), senderBuffer.totalDuplicateAck, senderBuffer.totalTimeouts, senderBuffer.totalRetransmitions);
        }
        else{
            if(status == FAST_RECOVERY)
                updateCwnd(MULTIPLICATIVE_INCREASE);
            sendNew(1);
            LOGI("Duplicate: %d, %ld %d, %d", pkt->header.ackNo, senderBuffer.buf[pkt->header.ackNo+1].time_, senderBuffer.buf[pkt->header.ackNo+1].rto_, pkt->header.origAckNo);
        }
    }
    return 0;
}

appInt BasicChannelHandler::sendNew(appInt16 cnt) {
    appInt16 unsent, sntCnt;
    unsent = senderBuffer.maxPacket - senderBuffer.maxSent;
    for(sntCnt = 0; cansend() && unsent && sntCnt < cnt; sntCnt ++){
        appInt16 nextPktSeq = senderBuffer.maxSent + 1;
        sendSeq(nextPktSeq);
        senderBuffer.maxSent += 1;
        senderBuffer.totalSent += 1;
        unsent -= 1;
    }
    return sntCnt;
}

appSInt BasicChannelHandler::sendSeq(appInt16 seq) {
    sendSeqLock.lock();
    Packet *pkt = senderBuffer.buf[seq].pkt_;
    APP_ASSERT(seq == pkt->header.seqNo);
    appSInt64 time = getTime().getMicro();
    pkt->header.sentTS = time;
    LOGI("tracedata: %ld, %g, %ld, %d, %d, %d, %ld, %ld", time, cwnd, srtt, ssthresh, seq, id(), rttvar, rto);
    auto ret = send(pkt);
    senderBuffer.startTimer(seq, time, rto);
    sendSeqLock.unlock();
    return ret;
}

void BasicChannelHandler::sendAck(Packet *origPkt, appInt16 ackno) {
//    Packet *pkt = getPacketPool().getNewPacket();
    Packet npkt;
    Packet *pkt = &npkt;
    pkt->header.ackNo = ackno;
    pkt->header.flag = FLAG_ACK;
    pkt->header.flowId = origPkt->header.flowId;
    pkt->header.fingerPrint = origPkt->header.fingerPrint;
    pkt->header.window = APP_INT16_CNT/2;
    pkt->header.sentTS = origPkt->header.sentTS;
    pkt->header.origAckNo = origPkt->header.seqNo;
    send(pkt);
}

void BasicChannelHandler::waitToMoreSpace() {
    if(!haveCell())
        waitToEmptySapce.wait();
}

void BasicChannelHandler::shutDown() {
    packetLock.lock();
    closed = TRUE;
    packetLock.unlock();
}

void *BasicChannelHandler::forwardPacketsToIfcScheduler(void *data) {
    BasicChannelHandler *self = (BasicChannelHandler *)data;
    appInt16 nextPkt = self->senderBuffer.lastAcked + 1;
    appInt16 pktCnt = self->senderBuffer.maxPacket - self->senderBuffer.lastAcked;
    self->senderBuffer.timerList.clear();
//    auto ifcSch = dynamic_cast<InterfaceScheduler*>(parent);
//    APP_ASSERT(ifcSch)
    for(auto i = 0; i < pktCnt; i++, nextPkt++){
        if(self->senderBuffer[nextPkt].pkt_ == NULL)
            continue;
        auto pkt = self->senderBuffer[nextPkt].pkt_;
        self->senderBuffer[nextPkt].reset();
        if(pkt->header.flag&FLAG_CSN){
            getPacketPool().freePacket(pkt);
            continue;
        }
        self->parent->sendPacketTo(0, pkt);
    }
    return NULL;
}

void BasicChannelHandler::sendCSNPacket() {
    auto pkt = getPacketPool().getNewPacket();
    pkt->header.flag = FLAG_DAT|FLAG_CHS;
    pkt->header.fingerPrint = parent->getFingerPrint();
    sendPacket(pkt);
}

} //namespace channelHandler
