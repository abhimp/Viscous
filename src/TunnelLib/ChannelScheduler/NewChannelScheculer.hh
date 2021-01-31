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
 * NewChannelScheculer.hh
 *
 *  Created on: 22-Aug-2017
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_CHANNELSCHEDULER_NEWCHANNELSCHECULER_HH_
#define SRC_TUNNELLIB_CHANNELSCHEDULER_NEWCHANNELSCHECULER_HH_

#include <common.h>
#include <unistd.h>
#include <mutex>

#include "../../util/AppLLQueue.hh"
#include "../CommonHeaders.hh"
#include "../Packet.h"
#include "SchedulerInterface.hh"
#include "../InterfaceController/InterfaceMonitor.hh"
#include "../../util/AppThread.hh"
namespace channelHandler{
class ChannelHandler;
}
struct RemoteAddr;

namespace scheduler {

#define BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT 16
#define BASIC_CHANNEL_HANDLER_CHANNEL_COUNT 256
#define SCHEDULER_PACKET_QUEUE_SIZE 100



class NewChannelScheculer: public BaseReliableObj, public SchedulerInterface, public util::AppThread, public NetworkEventListner {
public:
    NewChannelScheculer(BaseReliableObj *parent, appBool doNotAddChannel = FALSE);
    virtual ~NewChannelScheculer();
    appSInt sendPacketTo(appInt id, Packet *pkt);
    appSInt recvPacketFrom(Packet *pkt, RecvSendFlags &flags);
    appInt timeoutEvent(appTs time);
    appStatus closeFlow(appFlowIdType flowId);
    void addRemote(appInt8 id, RemoteAddr *remoteAddr);
    void removeRemote(appInt8 id);
    void notifyFreeCell(appInt8 chId);
    void close();
    virtual void readyToSend(appInt8 chId, appInt ready = 1);
    virtual void run();
//Virtual functions from NetworkEventHandler
    virtual void interfaceAdded(appInt8 ifcLoc);
    virtual void interfaceRemoved(appInt8 ifcLoc);
    virtual void notifyDeadChannel(appInt8 chId);
    virtual appSInt sendNextPacketUsingCh(appInt8 chId);

private:
    void addChannel(appInt8 ifcLoc, appInt8 ifcRem);
    appSInt schedule();
    RemoteAddr *remoteAddresses[BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT];
//    channelHandler::ChannelHandler *channelHandlers[BASIC_CHANNEL_HANDLER_CHANNEL_COUNT]; // ease of access
    std::shared_ptr<channelHandler::ChannelHandler> channelHandlers[BASIC_CHANNEL_HANDLER_CHANNEL_COUNT];
    std::set<appInt8> activeChannelHandlesList;
    appInt readyToDeliverPacket[BASIC_CHANNEL_HANDLER_CHANNEL_COUNT];
    util::AppLLQueue<Packet> outgoingPacketQueue;
    util::AppLLQueue<Packet> undeliveredPacketQueue;
    util::AppMutex sendPacketLock;
    appBool doNotAddChannel;
    util::AppMutex readyToSendLock;
    util::AppMutex deadChannelLock;
    util::AppSemaphore readyToSendSem;
    appInt nextChid = 1;
    util::AppSemaphore waitForToken, waitForNewPacket;
    appBool waitToClose;
    Packet *controlPkt;
    util::AppLLQueue<Packet> closePktsQueue;
    util::AppMutex controlPktLock;
    appInt64 inPkt, outPkt;
//    InterfaceMonitor *ifcMon;
//    util::AppSemaphore sendPacketSemaphore;
//    util::AppPool<>
};

} /* namespace util */

#endif /* SRC_TUNNELLIB_CHANNELSCHEDULER_NEWCHANNELSCHECULER_HH_ */
