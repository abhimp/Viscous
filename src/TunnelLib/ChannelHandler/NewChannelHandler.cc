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
 * NewChannelHandler.cc
 *
 *  Created on: 07-Aug-2017
 *      Author: abhijit
 */

#include "NewChannelHandler.hh"
#include "../ChannelScheduler/SchedulerInterface.hh"

namespace channelHandler{

NewChannelHandler::NewChannelHandler(BaseReliableObj* parent, appInt8 ifcLoc,
        RemoteAddr& rmadr, appInt8 ifcSrc, appInt8 ifcDst, bool lock): SackNewRenoChannelHandler(parent, ifcLoc, rmadr, ifcSrc, ifcDst, lock)
        , chThreadId(0)
        , chStopThread(FALSE)
        , chThreadMutex()
        , ackedSentUpto(APP_INT16_MAX)
        , lastAckSent(getTime().getMicro())
        , shutDownSent(FALSE)
        , shutDownRecv(FALSE)
        , shutDownSentRecv(FALSE)
        , outgoingAckCount(0)
        , ackHeaderLL(NULL)
{
}

NewChannelHandler::~NewChannelHandler() {
    chStopThread = TRUE;
    waitForPacket.notify();
    waitToDie.wait();

    delete ackHeaderLL;
}

appSInt NewChannelHandler::sendPacket(Packet* pkt) {
    sendPacketLock.lock();
    if(closed){
        sendPacketLock.unlock();
        return -9;
    }
    if(deadChannel){
        sendPacketLock.unlock();
        return -6;
    }
    if(!started and (pkt->header.flag|FLAG_CSN) == 0){
        sendPacketLock.unlock();
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
            sendPacketLock.unlock();
//            packetLock.unlock();
            return -1;
        }
        addBuffer(pkt);
    }
    sendPacketLock.unlock();
    return 0;
}

appInt NewChannelHandler::sendNew(appInt16 cnt) {
    auto x = sendNext(cnt);
    if(x == 0)
        return 0;
//    auto ifcSch = dynamic_cast<scheduler::SchedulerInterface *>(parent);
//    APP_ASSERT(ifcSch);
////    if(haveCell())
//    ifcSch->readyToSend(id(), x);
    return x;
}

Packet* NewChannelHandler::getUndeliveredPackets() {
    auto self = this;
    util::AppLLQueue<Packet>  pktQ;
    undeliveredPktLock.lock();
    appInt16 nextPkt = self->senderBuffer.lastAcked + 1;
    appInt16 pktCnt = self->senderBuffer.maxPacket - self->senderBuffer.lastAcked;
    self->senderBuffer.timerList.clear();
    for(auto i = 0; i < pktCnt; i++, nextPkt++){
        if(self->senderBuffer[nextPkt].pkt_ == NULL)
            continue;
        auto pkt = self->senderBuffer[nextPkt].pkt_;
        self->senderBuffer[nextPkt].reset();
        if(pkt->header.flag&FLAG_CSN){
            getPacketPool().freePacket(pkt);
            continue;
        }
        pktQ.addToQueue(pkt);
    }
    undeliveredPktLock.unlock();
    return pktQ.getAllFromQueue();
}

void NewChannelHandler::markDeadChannel() {
    deadChannel = TRUE;
    chStopThread = TRUE;
    waitForPacket.notify();
}

appInt NewChannelHandler::sendNext(appInt16 cnt) {
    sendNextLock.lock();
    appInt16 unsent, sntCnt;
    unsent = senderBuffer.maxPacket - senderBuffer.maxSent;
    for(sntCnt = 0; cansend() && unsent && sntCnt < cnt; sntCnt ++){
        appInt16 nextPktSeq = senderBuffer.maxSent + 1;
        sendSeq(nextPktSeq);
        senderBuffer.maxSent += 1;
        senderBuffer.totalSent += 1;
        unsent -= 1;
    }
    sendNextLock.unlock();
    return sntCnt;
}

appSInt NewChannelHandler::recv(Packet* pkt) {
    if(pkt->header.flag&FLAG_ACK){
        pkt->recvTS = getTime().getMicro();
        auto newPkt = getPacketPool().getNewPacketWithData();
        newPkt->cloneWithoutData(pkt);
        ackRecvQueue.addToQueue(newPkt);
        if(senderBuffer.buf[pkt->header.seqNo].pktSent_){
            senderBuffer.buf[pkt->header.seqNo].ackRcvd_ = 1;
        }
        waitForPacket.notify();
    }
    if(pkt->header.flag&FLAG_DAT){
        auto newPkt = getPacketPool().getNewPacketWithData();
        auto ptr = newPkt->data;
        newPkt->cloneWithoutData(pkt);
        newPkt->data = pkt->data;
        pkt->data = ptr;
        pkt->len = 0;
        dataRecvQueue.addToQueue(newPkt);
        waitForPacket.notify();
    }
    pkt->processed = FALSE;
    pkt->accepted = FALSE;
//    validatePacket(pkt);
    return 0;
}

void NewChannelHandler::run() {
    SackNewRenoChannelHandler::startUp();
    while(1){
        waitForPacket.wait();
        if(ackRecvQueue.empty() and dataRecvQueue.empty() and !chStopThread)
            continue;

        if(chStopThread)
            break;
        if(!dataRecvQueue.empty())
            pktRecv();
        if(!ackRecvQueue.empty())
            ackRecv();
    }
    waitToDie.notify();
}

appInt NewChannelHandler::pktRecv() {
    Packet *pkt = NULL;
    auto packets = dataRecvQueue.getAllFromQueue();
    while(packets){
        pkt = packets;
        packets = (Packet *)packets->next;
        pktRecv(pkt);
        pkt->processed = 1;
        parent->recvProcPacket(pkt);
//        getPacketPool().freePacket(pkt);
    }
    return 0;
}

appInt NewChannelHandler::pktRecv(Packet* pkt) {
    if(!recieverBuff.recvStarted and !(pkt->header.flag&FLAG_CHS)){
        return -1;
    }
    if(pkt->header.flag&FLAG_CHS){
        recieverBuff.recvStarted = TRUE;
        started = TRUE;
    }
    appInt16 normalizedSeqNo = pkt->header.seqNo - recieverBuff.expectedSeq;
    appInt16 normalizedMaxRecv = recieverBuff.maxRecvd - recieverBuff.expectedSeq;
    LOGI("pkt rcvd: %ld, %d, %d, %d, %d", getTime().getMicro(), recieverBuff.expectedSeq-1, pkt->header.seqNo, normalizedSeqNo, id());
    appInt16 expSeq = recieverBuff.expectedSeq;
//    appInt16 normalizedAckToBeSend = pkt->header.seqNo - ackedSentUpto;
    lastRecvPkt = *pkt;
    if(pkt->header.flag&FLAG_CHF){
        shutDownRecv = TRUE;
        if(!shutDownSent){
            initShutdown();
//            shutDownLock.notify();
        }
        shutDownLock.notify();
//        closed = TRUE;
//        return -1;
    }
    if(normalizedSeqNo >= 0 and normalizedSeqNo < (APP_INT16_CNT/2))
    {
        if(!recieverBuff.buf.getBit(pkt->header.seqNo))
            pkt->accepted = TRUE;
        recieverBuff.buf.setBit(pkt->header.seqNo);
        for(; recieverBuff.buf.getBit(expSeq); expSeq++){
            recieverBuff.buf.resetBit(expSeq);
        }
        recieverBuff.expectedSeq = expSeq;
//        if(normalizedSeqNo > 0 and normalizedSeqNo >= MAX_PENDING_ACK){
            sendAck(pkt, expSeq-1);
//        }
        if(normalizedSeqNo > normalizedMaxRecv)
            recieverBuff.maxRecvd = pkt->header.seqNo;

    }
    else if(normalizedSeqNo > ((APP_INT16_CNT >> 1) + (APP_INT16_CNT >> 2))){ // check it is in forth quater
        sendAck(pkt, expSeq-1);
    }
    return 0;
}

appInt NewChannelHandler::ackRecv() {
    appInt16 maxAck = senderBuffer.lastAcked;
    auto packets = ackRecvQueue.getAllFromQueue();
    for(Packet *pkt = packets; pkt; pkt = (Packet*)pkt->next){
        appInt16 normalisedAckNo = pkt->header.ackNo - senderBuffer.lastAcked;

        appInt16 normalisedMaxAckNo = maxAck - senderBuffer.lastAcked;

        if(normalisedAckNo > normalisedMaxAckNo){
            maxAck = pkt->header.ackNo;
        }
    }

    sendPacketLock.lock();
    auto startedWith = senderBuffer.lastAcked;
    appInt16 normalisedMaxAckNo = maxAck - senderBuffer.lastAcked;
    while(packets){
        auto pkt = packets;
        packets = (Packet *)packets->next;
        appInt16 normalisedOrigAckNo = pkt->header.origAckNo - senderBuffer.lastAcked;
        if(pkt->header.ackNo == maxAck){
            ackRecv(pkt);
        }
        else if(normalisedOrigAckNo > normalisedMaxAckNo){
            ackRecv(pkt);
        }
        getPacketPool().freePacket(pkt);
    }
    if(shutDownSentRecv)
        shutDownLock.notify();
    auto endedWith = senderBuffer.lastAcked;
    sendPacketLock.unlock();

    appInt16 vacantCell = endedWith - startedWith;
    auto ifcSch = dynamic_cast<scheduler::SchedulerInterface *>(parent);
    APP_ASSERT(ifcSch);
//    if(haveCell())
    ifcSch->readyToSend(id(), vacantCell);
    return 0;
}

appInt NewChannelHandler::ackRecv(Packet* pkt) {
    if(pkt->header.window != 0){
        senderWindow = pkt->header.window;
    }
    appInt16 normalisedackno = pkt->header.ackNo - senderBuffer.lastAcked;
    if(!senderBuffer.started)
        return 0;
    appBool newAck = FALSE;
    appBool partialAck = FALSE;
    appBool duplicateAck = FALSE;
    auto time = getTime();
    if(normalisedackno > 0 and normalisedackno <= APP_INT16_CNT/2) // received expected ack
    {
        if((status == FAST_RECOVERY or status == MULTI_RECOVERY) and normalisedackno < recover){
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

    appInt16 origAckNo = pkt->header.origAckNo;
    if(pkt->header.ackNo != origAckNo and senderBuffer.buf[origAckNo].pkt_){
        senderBuffer.stopTimer(origAckNo);
        freePacket(senderBuffer.buf[origAckNo].pkt_);
        senderBuffer.buf[origAckNo].reset();
    }

    senderBuffer.totalSuccessfullySent += normalisedackno;

    if(newAck or partialAck) // received expected ack
    {
        appInt16 acked = senderBuffer.lastAcked + 1;
        senderBuffer.lastAcked = pkt->header.ackNo;
//        waitToEmptySapce.notify();
//        sendFreeCellNotification();
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
            if(senderBuffer.buf[tmpy].pkt_ == NULL)
                continue;
            senderBuffer.stopTimer(tmpy);
            if(shutDownSent and senderBuffer.buf[tmpy].pkt_->header.flag&FLAG_CHF){
                shutDownSentRecv = TRUE;
                shutDownLock.notify();
            }
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
                if(senderBuffer.buf[tmpy].pkt_)
                    senderBuffer.restartTimer(tmpy, time_);
            }

            if(status == FAST_RECOVERY){
                status = MULTI_RECOVERY;
                multiRecoverIndex = acked + 1;
                senderBuffer.stopTimer(acked);
                sendSeq(acked);
                duplicateCount = 3;
                senderBuffer.totalDuplicateAck ++;
                senderBuffer.totalRetransmitions ++;
                LOGI("Retransmission count: %ld, %d, %lu, %lu, %lu", time.getMicro(), id(), senderBuffer.totalDuplicateAck, senderBuffer.totalTimeouts, senderBuffer.totalRetransmitions);
            }
            else{
                sendNew(1);
                updateCwnd(SET_TO, cwnd - normalisedackno + 1);
            }
        }
    }
    else if(duplicateAck){
        duplicateCount += 1;
        if(status == MULTI_RECOVERY){
            for(;senderBuffer.buf[multiRecoverIndex].pkt_ == NULL and multiRecoverIndex <= recoverIndex; multiRecoverIndex++); //search for next un-acked packets
            if(multiRecoverIndex <= recoverIndex and senderBuffer.buf[multiRecoverIndex].pkt_ == NULL){
                senderBuffer.stopTimer(multiRecoverIndex);
                sendSeq(multiRecoverIndex); //as we are not sending any new Packet, window will not change
                multiRecoverIndex ++;
//                ssthresh = APP_MAX(ssthresh/2, 2); //Not sure redfining ssthresh is good or not.
                senderBuffer.totalDuplicateAck ++;
                senderBuffer.totalRetransmitions ++;
                LOGI("Retransmission count: %ld, %d, %lu, %lu, %lu", time.getMicro(), id(), senderBuffer.totalDuplicateAck, senderBuffer.totalTimeouts, senderBuffer.totalRetransmitions);
            }
            else{
                updateCwnd(MULTIPLICATIVE_INCREASE);
                sendNew(1);
            }
        }
        else{
            if(duplicateCount == 3){
                appInt16 nextPktSeq = senderBuffer.lastAcked + 1;
                senderBuffer.stopTimer(nextPktSeq);
                sendSeq(nextPktSeq);
                if(status != FAST_RECOVERY){
                    recover = senderBuffer.maxSent - senderBuffer.lastAcked;
                    recoverIndex = senderBuffer.maxSent;
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
            }
        }
        LOGI("Duplicate: %d, %ld %d, %d", pkt->header.ackNo, senderBuffer.buf[pkt->header.ackNo+1].time_, senderBuffer.buf[pkt->header.ackNo+1].rto_, pkt->header.origAckNo);
    }
    return 0;
}

appInt NewChannelHandler::timeoutEvent(appTs time) {
    sendPacketLock.lock();
    if(closed or deadChannel){
        sendPacketLock.unlock();
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
            if(senderBuffer.buf[expectedSeq].ackRcvd_)
                continue;
            if(expectedSeq == 0 and senderBuffer[expectedSeq].retryCnt_ > 2){
                LOGI("Channel dead: %d", id());
                deadChannel = TRUE;
                auto scheduler = dynamic_cast<scheduler::SchedulerInterface *>(parent);
                if(!scheduler){
                    LOGE("Invalid Scheduler");
                    return 0;
                }
                scheduler->notifyDeadChannel(id());
                chStopThread = TRUE;
                waitForPacket.wait();
                break;
                APP_ASSERT(0);
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
    sendPacketLock.unlock();
    return 0;
}

void NewChannelHandler::sendAck(Packet* origPkt, appInt16 ackno) {
    Packet npkt;
    Packet *pkt = &npkt;
    pkt->header.ackNo = ackno;
    pkt->header.flag = FLAG_ACK;
    pkt->header.flowId = origPkt->header.flowId;
    pkt->header.fingerPrint = origPkt->header.fingerPrint;
    pkt->header.window = APP_INT16_CNT/2;
    pkt->header.sentTS = origPkt->header.sentTS;
    pkt->header.origAckNo = origPkt->header.seqNo;
    outgoingAckCount ++;
    if(outgoingAckCount > SEND_CONTROL_AFTER){
        outgoingAckCount = 0;
        if(ackHeaderLL)
            delete ackHeaderLL;
        ackHeaderLL = parent->getReceiverStatus();
    }
    PacketReadHeader *ackHdr = ackHeaderLL;
//    for(appInt x = 0; x < 16 and ackHeaderLL; x++){
//        auto tmp = ackHeaderLL;
//        ackHeaderLL = (PacketReadHeader *)ackHeaderLL->next;
//        tmp->next = ackHdr;
//        ackHdr = tmp;
//    }

    if(ackHdr){
        pkt->optHeaders = ackHdr;
        pkt->header.flag |= FLAG_CTR;
        ackHeaderLL = NULL;
    }
    send(pkt);
    if(pkt->optHeaders)
        freeOptionalHeaderToPool(pkt->optHeaders);

//    getPacketPool().freePacket(pkt)
//    SackNewRenoChannelHandler::sendAck(origPkt, ackno);
    LOGI("Sending ACK %d\n", ackno);
    ackedSentUpto = ackno;
}

//bool NewChannelHandler::haveCell() {
//    return started and SackNewRenoChannelHandler::haveCell();;
//}

void NewChannelHandler::shutDown() {
    initShutdown();
    while(1){
        if(shutDownRecv and shutDownSent and shutDownSentRecv)
            break;
        shutDownLock.wait();
    }
}

void NewChannelHandler::initShutdown() {
    if(shutDownSent){
        return;
    }
    Packet *pkt = getPacketPool().getNewPacketWithData();
    pkt->header.fingerPrint = parent->getFingerPrint();
    pkt->header.flag = FLAG_DAT|FLAG_CHF;
    sendPacket(pkt);
    shutDownSent = TRUE;
    shutDownLock.notify();
    closed = TRUE;
}

void NewChannelHandler::startUp() {
//    startThread();
    start();
//    SackNewRenoChannelHandler::startUp();
}

inline void NewChannelHandler::updateRtt(Packet* pkt) {
    appSInt64 sentTime = pkt->header.sentTS;//senderBuffer.buf[pkt->header.ackNo].time_;
    if(sentTime == 0) return;
    if(pkt->recvTS)
        rtt = pkt->recvTS - sentTime;
    else
        rtt = getTime().getMicro() - sentTime;
    if(srtt == 0){
        srtt = rtt;
        rttvar = rtt/2;
        rto = srtt + APP_MAX(G, 4*rttvar);
    }
    else{
        rttvar = (1-beta) * rttvar + beta * APP_ABS(srtt - rtt);
        srtt = (1-alpha)*srtt + alpha*rtt;
        rto = srtt + APP_MAX(G, 4*rttvar);
    }
    APP_ASSERT((rto < (15*TIMER_SECOND_IN_US)))
}

appSInt NewChannelHandler::sendSeq(appInt16 seq) {
    sendSeqLock.lock();
    Packet *pkt = senderBuffer.buf[seq].pkt_;
    if(pkt == NULL)
        return -42;
    APP_ASSERT(seq == pkt->header.seqNo);
    appSInt64 time = getTime().getMicro();
    pkt->header.sentTS = time;
    senderBuffer.buf[seq].pktSent_ = 1;
    LOGI("tracedata: %ld, %g, %ld, %d, %d, %d, %ld, %ld", time, cwnd, srtt, ssthresh, seq, id(), rttvar, rto);
    auto ret = send(pkt);
    senderBuffer.startTimer(seq, time, rto);
    sendSeqLock.unlock();
    return ret;
}

void NewChannelHandler::updateCwnd(cwnd_update_type type, float value) {
    auto startedWith = (appInt)(cwnd+0.5);
    switch(type){
        case MULTIPLICATIVE_INCREASE:
            cwnd += 1;
            break;

        case ADDITIVE_INCREASE:
            cwnd += 1/cwnd;
            break;

        case SET_TO_ONE:
            cwnd = 1.;
            break;

        case SET_TO:
            cwnd = value;
            break;
    }

    if(cwnd > MAX_CONG_WINDOW)
        cwnd = MAX_CONG_WINDOW;
    if(cwnd < 1.)
        cwnd = 1.;

    auto endedWith = (appInt)(cwnd+0.5);
    if(endedWith > startedWith){
        appInt increament = endedWith - startedWith;
        auto ifcSch = dynamic_cast<scheduler::SchedulerInterface *>(parent);
        APP_ASSERT(ifcSch);
        ifcSch->readyToSend(id(), increament);
    }
}

} //namespace channelHandler
