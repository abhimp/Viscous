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
 * SackNewRenoChannelHandler.cpp
 *
 *  Created on: 04-Apr-2017
 *      Author: abhijit
 */

#include "SackNewRenoChannelHandler.h"
#include "PacketPool.h"

#define APP_MAX(x,y) ((x) > (y) ? (x) : (y))
#define APP_ABS(x) ((x) < 0 ? (-1)*(x) : (x))


SackNewRenoChannelHandler::SackNewRenoChannelHandler(BaseReliableObj *parent, appInt8 ifcLoc, RemoteAddr &rmadr, appInt8 ifcSrc, appInt8 ifcDst) :
        BasicChannelHandler(parent, ifcLoc, rmadr, ifcSrc, ifcDst), multiRecoverIndex(0), recoverIndex(0)
{
    ssthresh = MAX_CONG_WINDOW/2;
}

SackNewRenoChannelHandler::~SackNewRenoChannelHandler() {
}

appInt SackNewRenoChannelHandler::ackRecv(Packet* pkt) {
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
            if(senderBuffer.buf[tmpy].pkt_ == NULL)
                continue;
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

appInt SackNewRenoChannelHandler::sendAllTillRecovery() {
//    appInt16 lastAcked = senderBuffer.lastAcked + 1;
    return 0;
}

void SackNewRenoChannelHandler::sendSeq(appInt16 seq) {
    sendSeqLock.lock();
    Packet *pkt = senderBuffer.buf[seq].pkt_;
    if(pkt == NULL)
        return;
    APP_ASSERT(seq == pkt->header.seqNo);
    appSInt64 time = getTime().getMicro();
    pkt->header.sentTS = time;
    LOGI("tracedata: %ld, %g, %ld, %d, %d, %d, %ld, %ld", time, cwnd, srtt, ssthresh, seq, id(), rttvar, rto);
    send(pkt);
    senderBuffer.startTimer(seq, time, rto);
    sendSeqLock.unlock();
}

