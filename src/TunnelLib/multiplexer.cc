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
 * muiltiplexer.c
 *
 *  Created on: 04-Aug-2016
 *      Author: abhijit
 */


#include "multiplexer.hh"

#include "CommonHeaders.hh"
#include <iostream>
#include "FlowHandler/StopNWait.hh"
#include "FlowHandler/StreamHandler.hh"
#include "FlowHandler/NewFlowHandler.hh"
#include "PacketPool.hh"


appSInt Multiplexer::sendPacketTo(appInt id, Packet *pkt)
{
    if(pkt->header.flowId == 0){
        return -ERROR_INVALID_FLOW;
    }
    return this->parent->sendPacketTo(this->id, pkt);
}

inline void Multiplexer::processOptionalHeader(PacketOptionalAbstractHeader* pktOptHdr) {
    switch(pktOptHdr->type){
        case OPTIONAL_PACKET_HEADER_TYPE_ACK_HEADER:
            {
                PacketAckHeader *pktAckHdr = (PacketAckHeader *)pktOptHdr;
                appFlowIdType flowId;
                flowId.flowId = pktAckHdr->flowId;
                flowId.fingerPrint = fingerPrint;
                auto br = getChild(flowId);
                if(br){
                    br->recvAck(pktAckHdr);
                }
            }
            break;
        case OPTIONAL_PACKET_HEADER_TYPE_READ_HEADER:
            {
                auto rmHdr = (PacketReadHeader *)pktOptHdr;
                for(auto x = rmHdr->readerInfo; x; x = x->next){
                    appFlowIdType flowId;
                    flowId.flowId = x->flowId;
                    flowId.fingerPrint = fingerPrint;
                    auto br = getChild(flowId);
                    if(br){
                        br->recvAck(flowId, x->readUpto);
                    }
                }
            }
            break;
        default:
            APP_ASSERT("Invalid case");
    }
}

appSInt Multiplexer::recvPacketFrom(Packet *pkt, RecvSendFlags &flags)
{
    if(!pkt)
        return -ERROR_INVALID_PACKET;
    if(!pkt->header.flowId and !(pkt->header.flag&FLAG_CTR))
        return -ERROR_INVALID_FLOW;
//    if(pkt->header.flag&FLAG_DAT)
//        outPacketCount ++;

    if(pkt->optHeaders){
        PacketOptionalAbstractHeader *pktAckHdr = pkt->optHeaders;
        while(pktAckHdr){
            PacketOptionalAbstractHeader *tmp = pktAckHdr;
            pktAckHdr = pktAckHdr->next;
            processOptionalHeader(tmp);
        }
    }
    if(pkt->header.flag&FLAG_CTR){
        getPacketPool().freePacket(pkt);
        return 0;
    }
    appFlowIdType flowId;
    flowId.flowId = pkt->header.flowId;
    flowId.fingerPrint = pkt->header.fingerPrint;
    auto child = getChild(flowId);
    appSInt ret = 0;
    if(!pkt->accepted){
        getPacketPool().freePacket(pkt);
        return 0;
    }

    if(!child){
        acceptLock.lock();
        addNewFlow(pkt, pkt->src_addr, pkt->dest_addr);
        child = getChild(flowId);
        APP_ASSERT(child);
        pendingFlows.insert(flowId.clientFlowId);
        acceptSem.notify();
        flags.newFlow = TRUE;
        acceptLock.unlock();
    }
    if(child){
        ret = child->recvPacketFrom(pkt, flags);
    }

    if(ret){
        getPacketPool().freePacket(pkt);
    }
    return 0;
}

int Multiplexer::printStat(){
    return children.size();
}

appInt Multiplexer::timeoutEvent(appTs time){
    for(auto iter = children.begin(); iter != children.end(); ++iter){
        auto obj = iter->second;
        obj->timeoutEvent(time);
    }
    return 0;
}


appFlowIdType Multiplexer::getNewConnection(appFlowIdType flowId, CongType type){
    ReliabilityMod *obj;
    appFlowIdType zero;
    switch (type) {
        case STREAM_HANDLER:
            obj = new FlowHandler::StreamHandler(this, flowId);
            break;
        case NEW_FLOW_HANDLER:
            obj = new FlowHandler::NewFlowHandler(this, flowId);
            break;
        default:
            return zero;
            break;
    }
    appInt chId = obj->getId();
//    FlowHandler::Streamer *str = new FlowHandler::Streamer(obj, flowId);
    std::shared_ptr<ReliabilityMod> sp(obj);
    accessChildrenMap.lock();
    id2FlowMap[chId] = flowId;
    children[flowId.clientFlowId] = sp;
    accessChildrenMap.unlock();
    return flowId;
}

appSInt Multiplexer::readData(appFlowIdType flowId, appByte *data, appInt size){
    auto child = getChild(flowId);
    if(!child) return -ERROR_INVALID_FLOW;
    auto ret = child->readData(flowId, data, size);
    if(ret == 0){
        closeFlow(flowId);
    }
    outPacketCount ++;
    if(outPacketCount > 400){
        auto pkt = generateControlPacket();
        if(parent->sendPacketTo(id, pkt) < 0){
            getPacketPool().freePacket(pkt);
        }
        outPacketCount = 0;
    }
    return ret;
}

appSInt Multiplexer::sendData(appFlowIdType flowId, appByte *data, appInt dataLen){
    auto child = getChild(flowId);
    if(!child) return -ERROR_INVALID_FLOW;
    return child->sendData(flowId, data, dataLen);
}

inline appBool Multiplexer::flowExists(appFlowIdType flowId){
    return hasKey(children, flowId.clientFlowId);
}

std::shared_ptr<ReliabilityMod> Multiplexer::getChild(appFlowIdType flowId){
    std::shared_ptr<ReliabilityMod> obj = NULL;
    accessChildrenMap.lock();
    if(!flowId.flowId)
        goto ret;
    if(!hasKey(children, flowId.clientFlowId)){
        LOGI("ERROR");
        goto ret;
    }
    obj = children[flowId.clientFlowId];
ret:
    accessChildrenMap.unlock();
    return obj;
}
appFlowIdType Multiplexer::addNewFlow(Packet *pkt, sockaddr_in &src_addr, sockaddr_in &dest_addr){
    appFlowIdType flowId;
    flowId.flowId = pkt->header.flowId;
    flowId.fingerPrint = fingerPrint;
    if(!hasKey(children, flowId.clientFlowId)){
        getNewConnection(flowId);//new SimpleReliability(this);
    }
    return flowId;
}

appStatus Multiplexer::closeFlow(appFlowIdType flowId){
//    APP_ASSERT(hasKey(children, flowId.clientFlowId));
    auto obj = getChild(flowId);
    if(!obj)
        return APP_FAILURE;
    obj->closeFlow(flowId);
    accessChildrenMap.lock();
    auto objId = obj->getId();
    children.erase(flowId.clientFlowId);
    id2FlowMap.erase(objId);
    accessChildrenMap.unlock();
//    delete obj;
    return APP_SUCCESS;
}


Multiplexer::~Multiplexer() {
    APP_ASSERT(children.size()==0);
//    closedFlows.clear();
}

appStatus Multiplexer::close() {
    APP_ASSERT(children.size() == 0);

    return APP_SUCCESS;
}

appSInt Multiplexer::recvAck(appFlowIdType flowId, appInt16 flowFeqNo) {
    auto child = getChild(flowId);
    if(!child)
        return -1;
//    return child->recvAck(flowId, flowFeqNo);
    return 0;
}

appFlowIdType Multiplexer::acceptFlow() {
    acceptSem.wait();
    acceptLock.lock();
    auto it = pendingFlows.begin();
    appFlowIdType flowId;
    flowId.clientFlowId = *it;
    auto child = getChild(flowId);
    APP_ASSERT(child);
    pendingFlows.erase(flowId.clientFlowId);
    acceptLock.unlock();
    return flowId;
}

PacketReadHeader* Multiplexer::getReceiverStatus() {
    PacketReadHeader *hdr = NULL;
    appFlowIdType clflid;
    accessChildrenMap.lock();
    hdr = GET_OPTIONAL_PACKET_HEADER_TYPE_READ_HDR;
    for (auto it: this->children) {
        auto flowId = it.first;
        auto child = it.second;
        auto flowHndlr = child;
        clflid.clientFlowId = flowId;
        hdr->addInfo(clflid.flowId, flowHndlr->getReadUpto());
    }
    accessChildrenMap.unlock();
    return hdr;
}

Packet* Multiplexer::generateControlPacket() {
    auto x = getReceiverStatus();
    auto pkt = getPacketPool().getNewPacketWithData();
    pkt->header.flag = FLAG_CTR;
    pkt->optHeaders = x;
    return pkt;
}
