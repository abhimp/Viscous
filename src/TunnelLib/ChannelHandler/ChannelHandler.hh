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
 * InterfaceHnadler.hh
 *
 *  Created on: 06-Dec-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_CHANNELHANDLER_HPP_
#define SRC_TUNNELLIB_CHANNELHANDLER_HPP_
#include <common.h>
#include "../CommonHeaders.hh"
#include "../Connection.hh"
#include "../InterfaceController/SearchMac.hh"
#include "../InterfaceController/SendThroughInterface.h"

namespace channelHandler{

class ChannelHandler : public TimeoutObserver, public PacketFlags, public AppTimeClass{
public:
    ChannelHandler(BaseReliableObj *parent, appInt8 ifcLoc, RemoteAddr &rmadr, appInt8 ifcSrc, appInt8 ifcDst);
    virtual ~ChannelHandler();
    virtual appInt16 getSenderWindowSize();
    virtual appSInt sendPacket(Packet *pkt);
    virtual appSInt recv(Packet *pkt);
    virtual bool haveCell();
    virtual bool spaceInCwnd(){APP_ASSERT(0 and "Not implemented"); return false;};
    virtual appInt &id();
    virtual appInt timeoutEvent(appTs time);
    virtual appSInt64 getRTT() {APP_ASSERT(0 and "Not implemented"); return 0;}
//    virtual appTs getTime(){ appTime x; appGetSysTime(&x); appTs y(0,0); y.nsec() = x.tv_nsec; y.sec() = x.tv_sec; return y;}
//    virtual void setIfcId(appInt8 ifcSrc, appInt8 ifcDst);
    virtual void waitToMoreSpace() = 0;
    virtual void freePacket(Packet *pkt);
    virtual void sendFreeCellNotification();
    virtual void shutDown();
    virtual void initShutdown() {APP_ASSERT("NOT IMPLEMENTED")};
    virtual void startUp()=0;
    virtual Packet *getUndeliveredPackets() {APP_ASSERT("NOT IMPLEMENTMENTED" and 0); return NULL;};
    virtual void markDeadChannel() {APP_ASSERT("NOT IMPLEMENTMENTED" and 0);} ;
    virtual appInt8 getIfcSrc(){ return ifcSrc;}
    virtual appInt8 getIfcDst(){ return ifcDst;}
protected:
    virtual appSInt send(Packet *pkt);
    BaseReliableObj *parent;
    InterfaceMonitor *ifcMon;
private:
    SendThroughInterface *sender;
    RemoteAddr remoteAddr;
    appInt32 remIp;
    appInt8 ifcSrc, ifcDst;
    appInt id_;

};

} //namespace channelHandler
#endif /* SRC_TUNNELLIB_CHANNELHANDLER_HPP_ */
