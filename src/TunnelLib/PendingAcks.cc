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
 * PendingAcks.cpp
 *
 *  Created on: 15-Dec-2016
 *      Author: abhijit
 */

#include "PendingAcks.hh"

#include "PacketPool.hh"
//#include <iostream>

#define hasKey(_map, _key) (_map.find(_key) != _map.end())
PendingAcks::PendingAcks() :
        list(NULL)
        , begin(NULL)
        , listCapa(0)
        , acks()
        , mtx()
{

}

PendingAcks::~PendingAcks() {
    for(auto x = removeAck(); x; x = removeAck()){
        LOGE("flowId: %u ackNo: %u", x->flowId, x->ackNo);
        getPacketAckedHeaderPool().free(x);
    }
    appCondFree(list);
}

void PendingAcks::addAck(PacketAckHeader *pktHdr){
    if(!pktHdr)
        return;
    mtx.lock();
    PacketAckHeader *tmpHdr = NULL;
    appInt16 ackNo = 0;
#if 0
    if(!hasKey(acks, pktHdr->flowId)){
        acks[pktHdr->flowId] = pktHdr;
        goto RETURN;
    }
    tmpHdr = acks[pktHdr->flowId];
    ackNo = pktHdr->ackNo - tmpHdr->ackNo;
    if(ackNo > 0){
        acks[pktHdr->flowId] = pktHdr;
        getPacketAckedHeaderPool().free(tmpHdr);
    }
    else{
        getPacketAckedHeaderPool().free(pktHdr);
    }
#else
    appInt flowId = pktHdr->flowId;
    if(listCapa <= flowId){
        list = (PacketAckHeader **)appRealloc(list, sizeof(PacketAckHeader *)*(flowId+1));
        memset(list + listCapa, 0, sizeof(PacketAckHeader *)*(flowId + 1 - listCapa)); //I have to initialized to zero.
        listCapa = flowId + 1;
        APP_ASSERT(list);
    }
    else{
        tmpHdr = list[flowId];
    }
    if(!tmpHdr){
        list[flowId] = pktHdr;
        pktHdr->next = begin;
        begin = pktHdr;
        goto RETURN;
    }
    ackNo = pktHdr->ackNo - tmpHdr->ackNo;
    if(ackNo > 0)
        tmpHdr->ackNo = ackNo;
    getPacketAckedHeaderPool().free(pktHdr);
#endif
RETURN:
    mtx.unlock();
}

PacketAckHeader *PendingAcks::removeAck() {
    mtx.lock();
    PacketAckHeader *tmp = NULL;
#if 0
    auto beg = acks.begin();
    if(beg != acks.end()){
        tmp = beg->second;
        acks.erase(beg);
    }
#else
    if(begin){
        tmp = begin;
        APP_ASSERT(begin->type == OPTIONAL_PACKET_HEADER_TYPE_ACK_HEADER);
        begin = (PacketAckHeader *)begin->next;
        list[tmp->flowId] = NULL;
    }
#endif
    mtx.unlock();
    return tmp;
}

appBool PendingAcks::haveAck() {
    return begin!=NULL;
}
