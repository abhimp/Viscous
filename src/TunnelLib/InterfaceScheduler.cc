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
 * InterfaceScheduler.cpp
 *
 *  Created on: 07-Dec-2016
 *      Author: abhijit
 */

#include "InterfaceScheduler.h"
#include "CubicChannelHandler.hpp"
#include "BasicChannelHandler.h"
#include "SackNewRenoChannelHandler.h"

InterfaceScheduler::InterfaceScheduler(BaseReliableObj *parent, appBool doNotAddChannel):
    BaseReliableObj(parent), interfaces(), nextInterfaceId(0),
    next(0), packetBuffer(512), freeCellChiID(0), doNotAddChannel(doNotAddChannel),
    APP_LL_QUEUE_INIT_LIST(Packet)
{
    memset(remoteAddresses, 0, sizeof(remoteAddresses));
    memset(channelHandlers, 0, sizeof(channelHandlers));
}

InterfaceScheduler::~InterfaceScheduler() {
    close();
    for(auto i = 0; i < BASIC_CHANNEL_HANDLER_CHANNEL_COUNT; i++){
        if(channelHandlers[i]){
            delete channelHandlers[i];
            channelHandlers[i] = NULL;
        }
    }
    for(auto i = 0; i < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT; i ++){
        auto x = remoteAddresses[i];
        if(x)
            delete x;
        remoteAddresses[i] = NULL;
    }
}

appSInt InterfaceScheduler::schedulePacket(Packet* pkt) {
    APP_ASSERT(interfaces.size() and "No interface");
	pkt->header.flag |= FLAG_DAT;
	appInt8 iterCnt = 0;
	for(;iterCnt<interfaces.size() and !interfaces[next]->haveCell(); iterCnt++){
		next = (next + 1)%interfaces.size();
	}

	appInt ret = 0;
	if(iterCnt < interfaces.size()){
	    ret = interfaces[next]->sendPacket(pkt);
	    next = (next + 1)%interfaces.size();
	}
	else{
	    waitForFreeCell.wait();
	    ret = channelHandlers[freeCellChiID]->sendPacket(pkt);
	}
	if(ret >= 0)
	    return ret;
	return schedulePacket(pkt); //this need to be send successfully.
}

appSInt InterfaceScheduler::sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen){
    sendPacketMutex.lock();
    auto ret = schedulePacket(pkt);
    sendPacketMutex.unlock();
    return ret;
}

appSInt InterfaceScheduler::recvPacketFrom(Packet *pkt)
{
    appInt8 ifcLoc = pkt->header.ifcdst;
    appInt8 ifcRem = pkt->header.ifcsrc;
    APP_ASSERT(ifcRem < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT && ifcRem > 0 && ifcLoc < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT && ifcLoc > 0);
    appInt8 chId = (ifcLoc << 4) | (ifcRem&0x0f);
    ChannelHandler *ch = channelHandlers[chId];
    if(ch == NULL){
        if(remoteAddresses[ifcRem] == NULL)
        {
            RemoteAddr remoteAddr(pkt->src_addr.sin_addr, pkt->src_addr.sin_port);
            addRemote(ifcRem, &remoteAddr);
        }

        if(doNotAddChannel)
            addChannel(ifcLoc, ifcRem);
        ch = channelHandlers[chId];

        if(ch == NULL) return -1;
    }
    appSInt ret = ch->recv(pkt);
    return ret;
}

appInt InterfaceScheduler::timeoutEvent(appTs time)
{
    return 0;
}

appStatus InterfaceScheduler::closeFlow(appInt16 flowId)
{
    APP_RETURN_FAILURE
}

void InterfaceScheduler::addRemote(appInt8 ifcRem, RemoteAddr* remoteAddr) {
    if(!(ifcRem != 0 && ifcRem < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT && remoteAddresses[ifcRem] == NULL)){
        return;
    }
    RemoteAddr *tmp = new RemoteAddr(remoteAddr->ip, remoteAddr->port);
    remoteAddresses[ifcRem] = tmp;
    if(doNotAddChannel){ //server will not add any channel it self.
        return;
    }
    SendThroughInterface **local = getInterfaceSender();
    for(auto ifcLoc = 0; ifcLoc < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT; ifcLoc++){
        if(local[ifcLoc] == NULL)
            continue;
        addChannel(ifcLoc, ifcRem);
    }
}

void InterfaceScheduler::removeRemote(appInt8 id) {
    APP_ASSERT(0 && "Not implemented");
}

void InterfaceScheduler::notifyFreeCell(appInt8 chId) {
    freeCellChiID = chId;
    waitForFreeCell.notify();
}

void InterfaceScheduler::addChannel(appInt8 ifcLoc, appInt8 ifcRem) {
    APP_ASSERT(ifcRem != 0 && ifcRem < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT && remoteAddresses[ifcRem]);
    APP_ASSERT(ifcLoc != 0 && ifcLoc < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT && getInterfaceSender()[ifcLoc]);
    auto remote = remoteAddresses[ifcRem];
//    ChannelHandler *ch = new CubicChannelHandler(this, ifcLoc, *remote);//
//    ChannelHandler *ch = new BasicChannelHandler(this, ifcLoc, *remote);//(this, *local, *remote);
    ChannelHandler *ch = new SackNewRenoChannelHandler(this, ifcLoc, *remote, ifcLoc, ifcRem);//(this, *local, *remote);
    APP_ASSERT(ch);
//    ch->id() = nextInterfaceId++;
//    ch->setIfcId(ifcLoc, ifcRem);
    LOGD("Adding new channel");
    appInt8 channelId = ch->id();//(ifcLoc << 4) | (ifcRem&0x0f); // exactly as the packet header says :).
//    ch->id() = channelId;
    channelHandlers[channelId] = ch;
    interfaces.push_back(ch);
    timeoutProducer().attach(ch);
}

void InterfaceScheduler::close() {
    for(auto i = 0; i < BASIC_CHANNEL_HANDLER_CHANNEL_COUNT; i++){
        if(channelHandlers[i]){
            LOGI("Shutting Down channel handler")
            channelHandlers[i]->shutDown();
            timeoutProducer().detach(channelHandlers[i]);
        }
    }
}
