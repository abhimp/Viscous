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
 * ServerConnectionClientDetail.hh
 *
 *  Created on: 14-Aug-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_SERVERCONNECTIONCLIENTDETAIL_HPP_
#define SRC_TUNNELLIB_SERVERCONNECTIONCLIENTDETAIL_HPP_

#include <common.h>
#include <stddef.h>
#include <unistd.h>
#include <atomic>
#include <cassert>
#include <set>

#include "../util/AppLLQueue.hh"
#include "../util/AppThread.hh"
#include "../util/ConditonalWait.hh"
#include "appTypes.h"
#include "ChannelScheduler/NewChannelScheculer.hh"
#include "ChannelScheduler/InterfaceScheduler.hh"
#include "CommonHeaders.hh"
#include "InterfaceController/Addresses.hh"
#include "multiplexer.hh"
#include "Packet.h"
#include "PendingAcks.hh"

namespace server{

class ServerConnection;

class Client4Server:public BaseReliableObj, public util::AppThread{
public:
    Client4Server(appInt16 fingerPrint, Multiplexer *mux, ServerConnection *parent);
    virtual ~Client4Server();
    virtual appInt16 getFingerPrint(){return clientFingerPrint;}
    virtual appSInt sendPacketTo(appInt id, Packet *pkt);
    virtual appSInt recvPacketFrom(Packet *pkt, RecvSendFlags &flags);
    virtual appInt timeoutEvent(appTs time);
    virtual void setMux(Multiplexer *mux){this->mux = mux;};
    virtual appFlowIdType addNewFlow(Packet *pkt, sockaddr_in &src_addr, sockaddr_in &dest_addr);
    virtual appSInt readData(appFlowIdType flowId, appByte *data, appInt size);
    virtual appSInt sendData(appFlowIdType flowId, appByte *data, appInt size);
    virtual appStatus closeFlow(appFlowIdType flowId){return mux->closeFlow(flowId);};
    virtual void setupInterfaces(appInt8 ifcRem, sockaddr_in &remAddr);
    inline virtual appSInt recvAck(appFlowIdType flowId, appInt16 flowSeqNo) {assert(mux); return mux->recvAck(flowId, flowSeqNo);};
    appInt32 acceptFlow();
    void run();
    virtual inline PacketReadHeader *getReceiverStatus(){APP_ASSERT(mux); return mux->getReceiverStatus();}
    virtual inline appSInt recvProcPacket(Packet *pkt);
    inline appBool isClosed(){return closed and (!mux or mux->isClosed());}
    appTs getLastUsed() {return lastUsed;};
private:
    virtual void close();
    virtual appSInt recvPacketAfterThread(Packet *pkt, RecvSendFlags &flags);
    appInt16 clientFingerPrint;
    std::set<RemoteAddr> remoteAddr;
    Multiplexer *mux;
    appTs lastUsed;
    util::AppSemaphore waitToClose;
    ServerConnection *parent;
#ifdef NEW_CHANNEL_HANDLE
    scheduler::NewChannelScheculer *ifcSch;
#else
    scheduler::InterfaceScheduler *ifcSch;
#endif
//    PendingAcks pendAck;
    util::AppSemaphore recvSem;
    std::atomic_bool stopThread;
    util::AppLLQueue<Packet> recvQueue;
    appBool closed;
};

} // namespace server
#endif /* SRC_TUNNELLIB_SERVERCONNECTIONCLIENTDETAIL_HPP_ */
