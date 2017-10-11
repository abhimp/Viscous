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
 * StreamHandler.hh
 *
 *  Created on: 12-Dec-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_FlowHandler_STREAMHANDLER_HPP_
#define SRC_TUNNELLIB_FlowHandler_STREAMHANDLER_HPP_
#include "../CommonHeaders.hh"
#include "../../util/CircularBuffer.hh"
#include "../../util/BitField.hh"
#include "../../util/ConditonalWait.hh"
#include <queue>
#include "../../util/ThreadPool.hh"
#include "../../util/ToZeroCounter.hh"

namespace FlowHandler {

class StreamHandler: public ReliabilityMod {
public:
    StreamHandler(BaseReliableObj *parent, appFlowIdType flowId);
    virtual ~StreamHandler();

    virtual appSInt sendPacketTo(appInt id, Packet *pkt);
    virtual appSInt recvPacketFrom(Packet *pkt, RecvSendFlags &flags);
    virtual void initiateClosure();
//    virtual appSInt readData(appInt16 flowId, appByte *data, appInt size);
    virtual appSInt sendData(appFlowIdType flowId, appByte *data, appInt dataLen);
    virtual appStatus recvAck(PacketAckHeader *pktAckHdr);
    virtual appInt timeoutEvent(appTs time);
    virtual appStatus closeFlow(appFlowIdType flowId);
    virtual appSInt recvAck(appFlowIdType flowId, appInt16 flowFeqNo);
    virtual void setCallBack(EventType evt, void *info, void *func);
    void printStat();
    virtual appInt16 getReadUpto();

private:
    appInt16 getNextSeqNo();
    void sendClosePacket();
    void sendAck(appInt16 ackNo);
    static void *sendRecvdPacket(void *data);
//    void prepareAck();
    void finishClosing();
    static void* sendClosePacketInsideThread(void *data);
    void runInThreadPool(util::cb_func fn, void* data);

    util::CircularBuffer<appByte> recvBuffer, sendBuffer;
    util::BitField ackedBitField;
    appInt16 nextSeqNumber;
    Packet **recvQueue;
    appBool firstPacketReceived;
    appInt16 firstRecvSeq;
    appInt16 recvdPktUpto;

    appInt16 readPacketUpto, ackedPacketUpto; //LIMITED_MEMORY

    appInt16 recvAckedPacketOffset;
//    BitField ackedBuffer;
    util::ConditionalWait waitForSenderSlot;
    util::AppSemaphore waitForCloseReport;
    util::ConditionalWait waitToRead;
    appBool connectionClosed, closingInitiated;
    appSInt packetsPendingToRead;
    appSInt64 totalPacketsRecvd;
    appInt16 jobsInThread;
    newDataToFlow newDataCB;
    closingFlow closingFlowCB;
    void *newDataCBInfo, *closingFlowCBInfo;
    util::AppMutex limitedMemoryLock;
    util::AppMutex readPacketLock;
    util::AppMutex jobInThreadLock;
    util::AppMutex closingMutex;
    util::ConditionalWait waitToDestroy;
    util::WorkerThread *worker;
    util::ToZeroCounter<int> threadJobCounter;
};

} //namespace FlowHandler
#endif /* SRC_TUNNELLIB_FlowHandler_STREAMHANDLER_HPP_ */
