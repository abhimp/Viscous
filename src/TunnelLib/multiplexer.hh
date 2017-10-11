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
 * multiplexer.h
 *
 *  Created on: 04-Aug-2016
 *      Author: abhijit
 */

#ifndef TUNNELLIB_MULTIPLEXER_HPP_
#define TUNNELLIB_MULTIPLEXER_HPP_
#include <map>
#include "CommonHeaders.hh"
#include "FlowHandler/Streamer.hh"
//#include "../util/AppSmartPointer.hh"
#include <atomic>
#include <memory>

typedef appInt16 MuxHdr;

class Multiplexer : public BaseReliableObj{
public:
    Multiplexer(BaseReliableObj *base, appInt16 fingerPrint): BaseReliableObj(base)
            , fingerPrint(fingerPrint)
            , outPacketCount(0)
            , server(parent->isServer())
            , client(parent->isClient())
        {
        };
    virtual ~Multiplexer();
    virtual appSInt sendPacketTo(appInt id, Packet *pkt);
    virtual appSInt recvPacketFrom(Packet *pkt, RecvSendFlags &flags);
    virtual appInt timeoutEvent(appTs time);
    virtual appFlowIdType getNewConnection(appFlowIdType flowId, CongType type = NEW_FLOW_HANDLER);
//    inline appBool isValidFlow(appFlowIdType flowId){return hasKey(children, flowId);}
    virtual appFlowIdType addNewFlow(Packet *pkt, sockaddr_in &src_addr, sockaddr_in &dest_addr);
    virtual appSInt readData(appFlowIdType flowId, appByte *data, appInt size);
    virtual appSInt sendData(appFlowIdType flowId, appByte *data, appInt dataLen);
    virtual inline appBool flowExists(appFlowIdType flowId);
    int printStat();

    virtual appStatus closeFlow(appFlowIdType flowId);
    virtual appStatus close();
//    void getOption(APP_TYPE::APP_GET_OPTION optType, void *optionValue, appInt optionValueLen, void *returnValue = NULL, appInt returnValueLen = 0);
    virtual appSInt recvAck(appFlowIdType flowId, appInt16 flowFeqNo);
    void setFingerPrint(appInt16 fingerPrint){this->fingerPrint = fingerPrint;}
    appFlowIdType acceptFlow();
    virtual PacketReadHeader *getReceiverStatus();
    appBool isClosed(){return !children.size();}

private:
    std::shared_ptr<ReliabilityMod> getChild(appFlowIdType flowId);
    inline void processOptionalHeader(PacketOptionalAbstractHeader *pktOptHdr);
    Packet *generateControlPacket();
    appInt16 fingerPrint;
//    appFlowIdType readHeader(appByte *data, appInt dataLen);
    std::map<appInt32, std::shared_ptr<ReliabilityMod> > children;
    std::map<appInt, appFlowIdType> id2FlowMap;
//    std::set<appInt16> closedFlows;
    util::AppMutex accessChildrenMap;

    std::set<appInt32> pendingFlows;
    util::AppMutex acceptLock;
    util::AppSemaphore acceptSem;

    std::atomic<appInt16> outPacketCount;
    std::set<appInt32> flowsAccepted;
    appBool server, client;
};



#define MUX_HEADER_LEN 4

#endif /* TUNNELLIB_MULTIPLEXER_HPP_ */
