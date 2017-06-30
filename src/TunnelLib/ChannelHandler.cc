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
 * InterfaceHnadler.cpp
 *
 *  Created on: 06-Dec-2016
 *      Author: abhijit
 */

#include "ChannelHandler.hh"
#include "PacketPool.h"
#include <cstdlib>
#include <ctime>
#include "InterfaceScheduler.h"

//ChannelHandler::ChannelHandler(BaseReliableObj *parent, InterfaceAddr &intfc, RemoteAddr &rmadr, appInt localPort) :
//    sender(NULL), remoteAddr(rmadr), parent(parent), remIp(0), ifcSrc(0), ifcDst(0), id_(0)
//{
//    APP_ASSERT(0 and "deprecated: do not use this");
//    remIp = ntohl(rmadr.ip.s_addr);
//    sender = new SendThroughInterface(intfc.ifc, intfc.gwMac, intfc.ifcIp, localPort);
//    sender->init();
//}
//
//ChannelHandler::ChannelHandler(BaseReliableObj *parent, InterfaceAddr &intfc, RemoteAddr &rmadr):
//    sender(NULL), remoteAddr(rmadr), parent(parent), remIp(0), ifcSrc(0), ifcDst(0), id_(0)
//{
//    APP_ASSERT(0 and "deprecated: do not use this");
//    remIp = ntohl(rmadr.ip.s_addr);
//    sender = new SendThroughInterface(intfc.ifc, intfc.gwMac, intfc.ifcIp, intfc.localPort);
//    sender->init();
//}

ChannelHandler::ChannelHandler(BaseReliableObj* parent, appInt8 ifcLoc, RemoteAddr& rmadr, appInt8 ifcSrc, appInt8 ifcDst):
    parent(parent), sender(NULL), remoteAddr(rmadr), remIp(0), ifcSrc(ifcSrc), ifcDst(ifcDst), id_((ifcSrc << 4) | (ifcDst&0x0f))
{
    sender = getInterfaceSender()[ifcLoc];
    APP_ASSERT(sender);
}

ChannelHandler::~ChannelHandler() {
}

appInt &ChannelHandler::id(){
    return id_;
}

appSInt ChannelHandler::recv(Packet *pkt){
    //TODO
    return 0;
}

appInt16 ChannelHandler::getSenderWindowSize(){
	std::srand(std::time(0)); // use current time as seed for random generator
	return std::rand()%57;
}
appSInt ChannelHandler::sendPacket(Packet *pkt){
    return send(pkt);
}

void ChannelHandler::shutDown() {
    APP_ASSERT(0 and "NOT IMPLEMENTED IN DERIVED CLASS");
}

appSInt ChannelHandler::send(Packet *pkt)
{
//    appChar msg[2048];
//    appInt dlen = 0;
    PacketOptionalAbstractHeader *pktAckHdr = NULL;
	//TODO handle pending acks more efficiently
//	for(appInt i = 0; i < Packet::maxOptHeader; i++){
//	    PacketOptionalAbstractHeader *tmp = parent->pendingAcks().removeAck();
//	    if(!tmp)
//	        break;
//	    tmp->next = pktAckHdr;
//	    pktAckHdr =  tmp;
//	}
	pkt->header.ifcsrc = ifcSrc;
	pkt->header.ifcdst = ifcDst;
	pkt->optHeaders = pktAckHdr;
//	pkt->header.destIp = remIp;
//    dlen = encodeHeader(pkt, (appByte *)msg, sizeof(msg));
//    if(!dlen)
//        return 0;
//    return sender->sendPacket(msg, dlen, remoteAddr.ip, remoteAddr.port);
	return sender->sendPkt(pkt, remoteAddr.ip, remoteAddr.port);
}


bool ChannelHandler::haveCell(){
	return true;
}
appInt ChannelHandler::timeoutEvent(appTs time){
    return 0;
}

//void ChannelHandler::setIfcId(appInt8 ifcSrc, appInt8 ifcDst) {
//    this->ifcSrc = ifcSrc;
//    this->ifcDst = ifcDst;
//}

void ChannelHandler::freePacket(Packet* pkt) {
    if(pkt->header.flag&FLAG_DAT)
        parent->recvAck(pkt->header.flowId, pkt->header.flowSeqNo);
    getPacketPool().freePacket(pkt);
}

void ChannelHandler::sendFreeCellNotification() {
    auto ifcSch = dynamic_cast<InterfaceScheduler *>(parent);
    APP_ASSERT(ifcSch);
    ifcSch->notifyFreeCell(id());
}
