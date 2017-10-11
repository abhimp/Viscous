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
 * InterfaceScheduler.h
 *
 *  Created on: 07-Dec-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_INTERFACESCHEDULER_HH_
#define SRC_TUNNELLIB_INTERFACESCHEDULER_HH_

#include <common.h>
#include <unistd.h>
#include <mutex>
#include <vector>

#include "../../util/AppLLQueue.hh"
#include "../../util/CircularBuffer.hh"
#include "../../util/ConditonalWait.hh"
#include "../InterfaceController/InterfaceMonitor.hh"
#include "../ChannelHandler/ChannelHandler.hh"
#include "../CommonHeaders.hh"
#include "../Packet.h"
#include "SchedulerInterface.hh"

namespace scheduler {

#define BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT 16
#define BASIC_CHANNEL_HANDLER_CHANNEL_COUNT 256

class InterfaceScheduler : public BaseReliableObj, public scheduler::SchedulerInterface, public NetworkEventListner {
public:
    InterfaceScheduler(BaseReliableObj *parent, appBool doNotAddChannel = FALSE);
    virtual ~InterfaceScheduler();
    appSInt sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen);
    appSInt recvPacketFrom(Packet *pkt, RecvSendFlags &flags);
    appInt timeoutEvent(appTs time);
    appStatus closeFlow(appFlowIdType flowId);
    void addRemote(appInt8 id, RemoteAddr *remoteAddr);
    void removeRemote(appInt8 id);
    void notifyFreeCell(appInt8 chId);
    void close();
    virtual void readyToSend(appInt8 chId, appInt ready = 1){APP_ASSERT(0);};
//Virtual functions from NetworkEventHandler
    virtual void interfaceAdded();
    virtual void interfaceRemoved(appInt8 ifcId);
//=================
private:
    inline void addChannel(appInt8 ifcLoc, appInt8 ifcRem);
    inline appSInt schedulePacket(Packet *pkt);
    std::vector<channelHandler::ChannelHandler *> interfaces;
    appInt nextInterfaceId;
    appInt next;
    util::CircularBuffer<Packet *> packetBuffer;
    RemoteAddr *remoteAddresses[BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT];
    channelHandler::ChannelHandler *channelHandlers[BASIC_CHANNEL_HANDLER_CHANNEL_COUNT]; // ease of access
    util::ConditionalWait waitForFreeCell;
    appInt8 freeCellChiID;
    appBool doNotAddChannel;
    util::AppMutex sendPacketMutex;
    util::AppLLQueue<Packet> packetQueue;
};

}
#endif /* SRC_TUNNELLIB_INTERFACESCHEDULER_HH_ */
