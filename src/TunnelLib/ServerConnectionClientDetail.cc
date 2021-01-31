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
 * ServerConnectionClientDetail.cc
 *
 *  Created on: 14-Aug-2016
 *      Author: abhijit
 */
#include "ServerConnectionClientDetail.hh"

#include "PacketPool.hh"
#include "ServerConnection.hh"
#include "InterfaceController/SendThroughInterface.h"


namespace server{
//===================
//  Client4Server
//===================
Client4Server::Client4Server(appInt16 fingerPrint, Multiplexer *mux, ServerConnection *parent): BaseReliableObj(parent)
        , clientFingerPrint(fingerPrint)
        , remoteAddr()
        , mux(mux)
        , lastUsed()
        , parent(parent)
        , ifcSch(NULL)
        , stopThread(FALSE)
        , closed(FALSE)
{
#ifdef NEW_CHANNEL_HANDLE
    ifcSch = new scheduler::NewChannelScheculer(this, TRUE);
#else
    ifcSch = new scheduler::InterfaceScheduler(this, TRUE);
#endif
}

Client4Server::~Client4Server() {
    if(ifcSch){
        delete ifcSch;
        ifcSch = NULL;
    }
    if(mux){
        delete mux;
        mux = NULL;
    }
    stopThread = TRUE;
    recvSem.notify();
    waitToClose.wait();
    PROFILER_DUMP
    PROFILER_RESET
}

void Client4Server::setupInterfaces(appInt8 ifcRem, sockaddr_in &remAddr){
    RemoteAddr rm(remAddr.sin_addr, remAddr.sin_port);
    ifcSch->addRemote(ifcRem, &rm);
}

appSInt Client4Server::sendPacketTo(appInt id, Packet *pkt){
    APP_ASSERT(pkt);
    if((pkt->header.flag&FLAG_CTR) and !ifcSch)
        return -ERROR_NETWORK_CLOSED;

    pkt->header.fingerPrint = clientFingerPrint;
    appSInt ret = ifcSch->sendPacketTo(id, pkt);
    return ret;
}

inline appSInt Client4Server::recvProcPacket(Packet* pkt) {
    APP_ASSERT(pkt->processed);
    START_PACKET_PROFILER_CLOCK(pkt)
    recvQueue.addToQueue(pkt);
    recvSem.notify();
    return 0;
}

appSInt Client4Server::recvPacketFrom(Packet *pkt, RecvSendFlags &flags){
    APP_ASSERT(pkt);
    LOGI("flowId recv %d", pkt->header.flowId);
    appSInt ret = 0;
    if(pkt->header.fingerPrint != clientFingerPrint){
        ret = 1;
        goto out;
    }
    if(!this->mux){
        ret = 2;
        goto out;
    }
    START_PACKET_PROFILER_CLOCK(pkt)
    recvQueue.addToQueue(pkt);
    recvSem.notify();
    return 0;
out:
    if(ret){
        getPacketPool().freePacket(pkt);
    }

    return 0;
}

appFlowIdType Client4Server::addNewFlow(Packet *pkt, sockaddr_in &src_addr, sockaddr_in &dest_addr){
    appFlowIdType default_flow;
    if(pkt->header.fingerPrint != clientFingerPrint)
        return default_flow;
    if(!this->mux)
        return default_flow;

    return mux->addNewFlow(pkt, src_addr, dest_addr);
}


appSInt Client4Server::readData(appFlowIdType flowId, appByte *data, appInt size){
    return mux->readData(flowId, data, size);
}

appSInt Client4Server::sendData(appFlowIdType flowId, appByte *data, appInt size){
    return mux->sendData(flowId, data, size);
}

appInt Client4Server::timeoutEvent(appTs time){
    if(mux)
        mux->timeoutEvent(time);
    return 0;
}

void Client4Server::close() {
    LOGI("ifcSch closing");
    if(ifcSch){
        ifcSch->close();
        delete ifcSch;
        ifcSch = NULL;
    }
    lastUsed = getTime();
}

appInt32 Client4Server::acceptFlow() {
    auto flowId = mux->acceptFlow();
    APP_ASSERT(flowId.fingerPrint == clientFingerPrint);
    return flowId.clientFlowId;
}

void Client4Server::run() {
    while(1){
        recvSem.wait();
        if(stopThread){
            waitToClose.notify();
            return;
        }
        auto pkt = recvQueue.getFromQueue();
        STOP_PACKET_PROFILER_CLOCK(pkt)
        if(pkt->header.flag&FLAG_CFN){
//            closeClient(client);
            close();
            closed = TRUE;
            Packet *npkt = getPacketPool().getNewPacket();
            npkt->header.flag = FLAG_CFN|FLAG_ACK;
            auto ifcMon = parent->getInterfaceMontor();
            ifcMon->get(ifcMon->getPrimaryInterfaceId())->sendPkt(npkt, pkt->src_addr.sin_addr, pkt->src_addr.sin_port);
            getPacketPool().freePacket(npkt);
            getPacketPool().freePacket(pkt);
            continue;
        }
        RecvSendFlags flag = DEFALT_RECVSENDFLAG_VALUE;
        recvPacketAfterThread(pkt, flag);
        if(flag.newFlow){
            parent->newFlowNoticfication(clientFingerPrint);
        }
        //TODO perform task
    }
}

appSInt Client4Server::recvPacketAfterThread(Packet* pkt,
        RecvSendFlags& flags) {
        appSInt ret = 0;
        if(pkt->header.ifcdst and pkt->header.ifcsrc){
            if(!pkt->processed and ifcSch)
                ifcSch->recvPacketFrom(pkt, flags);
            ret = mux->recvPacketFrom(pkt, flags);
        }
        if(ret)
            getPacketPool().freePacket(pkt);
        return 0;
}


} //namespace server
