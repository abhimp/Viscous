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
 * muiltiplexer.c
 *
 *  Created on: 04-Aug-2016
 *      Author: abhijit
 */


#include "multiplexer.hh"

#include "ARQ/StopNWait.hh"
#include "ARQ/StreamHandler.hh"

#include "CommonHeaders.hh"
#include "PacketPool.h"
#include <iostream>

MuxHdr Multiplexer::readHeader(appByte *data, appInt dataLen)
{
	MuxHdr flowId = 0;//{0,0};
	appInt16 temp;
	if(dataLen < MUX_HEADER_LEN){
		return flowId;
	}
	memcpy(&temp, data, 2);
	flowId = ntohs(temp);

	return flowId;
}

appSInt Multiplexer::sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen)
{
	if(pkt->header.flowId == 0){
	    return 1;
	}
	return this->parent->sendPacketTo(this->id, pkt, dest_addr, addrlen);
}

inline void Multiplexer::processOptionalHeader(
        PacketOptionalAbstractHeader* pktOptHdr) {
    switch(pktOptHdr->type){
        case OPTIONAL_PACKET_HEADER_TYPE_ACK_HEADER:
            {
                PacketAckHeader *pktAckHdr = (PacketAckHeader *)pktOptHdr;
                appInt16 flowId = pktAckHdr->flowId;
                ReliabilityMod *br = getChild(flowId);
                if(br){
                    br->recvAck(pktAckHdr);
                }
            }
	        break;
        default:
            APP_ASSERT("Invalid case");
    }
}

appSInt Multiplexer::recvPacketFrom(Packet *pkt)
{
	if(!pkt)
		return 1;
	if(pkt->optHeaders){
	    PacketOptionalAbstractHeader *pktAckHdr = pkt->optHeaders;
	    while(pktAckHdr){
	        PacketOptionalAbstractHeader *tmp = pktAckHdr;
	        pktAckHdr = pktAckHdr->next;
	        processOptionalHeader(tmp);
	    }
	}
	MuxHdr flowId;
	flowId = pkt->header.flowId;
	BaseReliableObj *child = getChild(flowId);
	appSInt ret = 0;
	if(!child){
	    ret = 2;
	}
	else if(!pkt->accepted){
	    ret = 2;
	}
	else{
	    ret = child->recvPacketFrom(pkt);
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
		BaseReliableObj *obj = iter->second->getObj();
		obj->timeoutEvent(time);
	}
	return 0;
}


ARQ::Streamer *Multiplexer::getNewConnection(appInt16 flowId, CongType type){
	ReliabilityMod *obj;
	switch (type) {
		case STREAM_HANDLER:
		    obj = new StreamHandler(this, flowId);
		    break;
		default:
			return NULL;
			break;
	}
	appInt chId = obj->getId();
	ARQ::Streamer *str = new ARQ::Streamer(obj, flowId);
	id2FlowMap[chId] = flowId;
	children[flowId] = str;
	return str;
}

appSInt Multiplexer::readData(appInt16 flowId, appByte *data, appInt size){
	BaseReliableObj *child = getChild(flowId);
	if(!child) return -2;
	return child->readData(flowId, data, size);
}

appSInt Multiplexer::sendData(appInt16 flowId, appByte *data, appInt dataLen){
	BaseReliableObj *child = getChild(flowId);
	if(!child) return -2;
	return child->sendData(flowId, data, dataLen);
}

inline appBool Multiplexer::flowExists(appInt16 flowId){
    return hasKey(children, flowId);
}

ReliabilityMod *Multiplexer::getChild(appInt16 flowId){
    ReliabilityMod *obj = NULL;
    accessChildrenMap.lock();
	if(!flowId)
		goto ret;
	if(!hasKey(children, flowId)){
		LOGI("ERROR");
		goto ret;
	}
	obj = children[flowId]->getObj();
ret:
	accessChildrenMap.unlock();
	return obj;
}
ARQ::Streamer *Multiplexer::addNewFlow(Packet *pkt, sockaddr_in &src_addr, sockaddr_in &dest_addr){
	MuxHdr flowId = pkt->header.flowId;
	if(hasKey(children, flowId)){
		return children[flowId];
	}
	auto *obj = this->getNewConnection(flowId);//new SimpleReliability(this);
	return obj;
}

appStatus Multiplexer::closeFlow(appInt16 flowId){
//    accessChildrenMap.lock();
    if(!hasKey(children, flowId))
        APP_RETURN_FAILURE;
    if(hasKey(closedFlows, flowId))
        APP_RETURN_SUCCESS;
    closedFlows.insert(flowId);
    auto obj = children[flowId];
	BaseReliableObj *child = obj->getObj();
	auto objId = child->getId();
    child->closeFlow(flowId);
    accessChildrenMap.lock();
	children.erase(flowId);
	id2FlowMap.erase(objId);
	delete child;
	delete obj;
    accessChildrenMap.unlock();
	return APP_SUCCESS;
}

void Multiplexer::getOption(APP_TYPE::APP_GET_OPTION optType, void* optionValue,
        appInt optionValueLen, void* returnValue, appInt returnValueLen) {
    APP_ASSERT(optionValue and optionValueLen);
    switch(optType){
        case APP_TYPE::APP_GET_STREAM_FLOW:
            {
                APP_ASSERT(sizeof(appInt16) == optionValueLen);
                ARQ::Streamer *child;
                appInt16 flowId;
                flowId = *((appInt16*)optionValue);
                if(!hasKey(children, flowId)){
                    return;
                }
                child = children[flowId];
                APP_ASSERT(child);
                if(!returnValue or !returnValueLen)
                    return;
                *((ARQ::Streamer **)returnValue) = child;
            }
            break;
        default:
            APP_ASSERT(0);
    }
}

Multiplexer::~Multiplexer() {
    closedFlows.clear();
}

appStatus Multiplexer::close() {
    std::set<appInt16> flows;
    for(auto it : children){
        flows.insert(it.first);
    }
    for(auto flow:flows){
        closeFlow(flow);
    }
//    sleep(1);
    return APP_SUCCESS;
}

appSInt Multiplexer::recvAck(appInt16 flowId, appInt16 flowFeqNo) {
	BaseReliableObj *child = getChild(flowId);
	if(!child)
	    return -1;
	return child->recvAck(flowId, flowFeqNo);
}
