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
 * NewChannelHandler.hh
 *
 *  Created on: 07-Aug-2017
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_CHANNELHANDLER_NEWCHANNELHANDLER_HH_
#define SRC_TUNNELLIB_CHANNELHANDLER_NEWCHANNELHANDLER_HH_

#include <mutex>
#include <appThread.h>
#include "SackNewRenoChannelHandler.hh"
#include "../../util/AppThread.hh"

#define MAX_PENDING_ACK 10

namespace channelHandler{
#define PKT_EVENT_TYPE_ENUM_PREF(_x) PKT_EVENT_TYPE_##_x
#define PKT_EVENT_TYPE_ENUMS(_) _(UNKNOWN), _(RCV), _(SNT), _(TOTAL)
struct ChannelEvent: public util::LL_Node{
    enum pktEvntTyp { PKT_EVENT_TYPE_ENUMS(PKT_EVENT_TYPE_ENUM_PREF)};
    pktEvntTyp evnt;
};

#define SEND_CONTROL_AFTER 60


class NewChannelHandler: public SackNewRenoChannelHandler, public util::AppThread {
public:
    NewChannelHandler(BaseReliableObj *parent, appInt8 ifcLoc, RemoteAddr &rmadr, appInt8 ifcSrc, appInt8 ifcDst, bool lock = true);
    virtual ~NewChannelHandler();

    virtual appInt ackRecv(Packet *pkt);
    virtual appInt ackRecv();
    virtual Packet *getUndeliveredPackets();
    virtual void initShutdown();
    virtual void markDeadChannel();
    virtual appInt pktRecv(Packet *pkt);
    virtual appInt pktRecv();
    virtual appSInt recv(Packet *pkt);
    virtual void run();
    virtual void sendAck(Packet *origPkt, appInt16 ackno);
    virtual appInt sendNew(appInt16 cnt = 1);
    virtual appSInt sendPacket(Packet *pkt);
    virtual void shutDown();
    virtual void startUp();
    virtual appInt timeoutEvent(appTs time);
    inline void updateRtt(Packet *pkt);
    virtual appSInt64 getRTT() {return srtt;}
    virtual bool spaceInCwnd();
//    virtual bool haveCell();
protected:
    virtual inline void updateCwnd(cwnd_update_type type, float value = 0.);
    virtual appInt sendNext(appInt16 cnt = 1);
    virtual appSInt sendSeq(appInt16 seq);
    appThreadInfoId chThreadId;
    appBool chStopThread;
    util::AppMutex chThreadMutex;
    util::AppLLQueue<Packet> dataRecvQueue;
    util::AppLLQueue<Packet> ackRecvQueue;
    util::AppSemaphore waitToDie;
    util::AppSemaphore waitForPacket;
    appInt16 ackedSentUpto;
    appSInt64 lastAckSent;
    Packet lastRecvPkt;
    util::AppMutex sendPacketLock;
    util::AppMutex recvPacketLock;
    util::AppMutex sendNextLock;
    util::AppMutex undeliveredPktLock;
    util::AppSemaphore shutDownLock;
//    appBool shutDownInitiated;
    appBool shutDownSent;
    appBool shutDownRecv;
    appBool shutDownSentRecv;
    util::AppLLQueue<ChannelEvent> eventQueue;
    appInt8 outgoingAckCount;
    PacketReadHeader *ackHeaderLL;
    enum task{};


    static util::AppPool<ChannelEvent> ChannelEventPool;
//    void bigLoop();
//    virtual void startThread();
};

} //namespace channelHandler

#endif /* SRC_TUNNELLIB_CHANNELHANDLER_NEWCHANNELHANDLER_HH_ */
