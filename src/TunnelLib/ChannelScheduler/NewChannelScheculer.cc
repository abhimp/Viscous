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
 * NewChannelScheculer.cc
 *
 *  Created on: 22-Aug-2017
 *      Author: abhijit
 */

#include "NewChannelScheculer.hh"

#include <netinet/in.h>
#include <cstdlib>
#include <cstring>

#include "../ChannelHandler/NewChannelHandler.hh"
#include "../InterfaceController/Addresses.hh"
#include "../InterfaceController/SendThroughInterface.h"

namespace scheduler {

NewChannelScheculer::NewChannelScheculer(BaseReliableObj* parent, appBool doNotAddChannel) : BaseReliableObj(parent)
        , outgoingPacketQueue(SCHEDULER_PACKET_QUEUE_SIZE)
        , doNotAddChannel(doNotAddChannel)
        , nextChid(1)
        , waitToClose(FALSE)
//        , ifcMon(ifcMon)
        , controlPkt(NULL)
        , inPkt(0)
        , outPkt(0)
{
    memset(remoteAddresses, 0, sizeof(remoteAddresses));
    memset(channelHandlers, 0, sizeof(channelHandlers));
    memset(readyToDeliverPacket, 0, sizeof(readyToDeliverPacket));
    start();
}

NewChannelScheculer::~NewChannelScheculer() {
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

appSInt NewChannelScheculer::sendPacketTo(appInt id, Packet* pkt) {
//    sendPacketLock.lock();
//    sendPacketLock.unlock();
    appBool notify = TRUE;
    pkt->header.flag |= FLAG_DAT;
    if(pkt->header.flag & FLAG_CTR){
        controlPktLock.lock();
        if(controlPkt){
            getPacketPool().freePacket(controlPkt);
            notify = FALSE;
        }
        controlPkt = pkt;
        controlPktLock.unlock();
    }
    else
        outgoingPacketQueue.addToQueue(pkt);
    if(notify)
        waitForNewPacket.notify();
    return 0;
//    schedule();
}

appSInt NewChannelScheculer::recvPacketFrom(Packet* pkt, RecvSendFlags& flags) {
    appInt8 ifcLoc = pkt->header.ifcdst;
    appInt8 ifcRem = pkt->header.ifcsrc;
    if(pkt->header.flag&FLAG_IFC){
        for(auto optHdr = pkt->optHeaders; optHdr; optHdr = optHdr->next){
            if(optHdr->type != OPTIONAL_PACKET_HEADER_TYPE_IP_CHANGE)
                continue;
            auto ipChngHdr = (PacketIpChngHeader *)optHdr;
            if(ipChngHdr->removed)
                removeRemote(ipChngHdr->ifcId);
            if(ipChngHdr->added)
                APP_ASSERT(0);
        }
    }
    APP_ASSERT(ifcRem < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT && ifcRem > 0 && ifcLoc < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT && ifcLoc > 0);
    appInt8 chId = (ifcLoc << 4) | (ifcRem&0x0f);
    channelHandler::ChannelHandler *ch = channelHandlers[chId];
    if(ch == NULL){
        if(pkt->header.flag&FLAG_IFC)
            return -3;
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

appInt NewChannelScheculer::timeoutEvent(appTs time) {
    return 0;
}

appStatus NewChannelScheculer::closeFlow(appFlowIdType flowId) {
    return APP_FAILURE;
}

void NewChannelScheculer::addRemote(appInt8 ifcRem, RemoteAddr* remoteAddr) {
    if(!(ifcRem != 0 && ifcRem < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT && remoteAddresses[ifcRem] == NULL)){
        return;
    }
    RemoteAddr *tmp = new RemoteAddr(remoteAddr->ip, remoteAddr->port);
    remoteAddresses[ifcRem] = tmp;
    if(doNotAddChannel){ //server will not add any channel it self.
        return;
    }
//    SendThroughInterface **local = getInterfaceSender();
    auto ifcMon = parent->getInterfaceMontor();
    for(auto ifcLoc = 0; ifcLoc < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT; ifcLoc++){
        if((!ifcMon || (*ifcMon)[ifcLoc] == NULL) and getInterfaceSender()[ifcLoc] == NULL)
            continue;
        addChannel(ifcLoc, ifcRem);
    }
}

void NewChannelScheculer::removeRemote(appInt8 ifcRem) {
    for(appInt8 ifcLoc = 0; ifcLoc < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT; ifcLoc++){
        appInt8 chId = (ifcLoc << 4) | (ifcRem&0x0f);
        auto ch = channelHandlers[chId];
        if(!ch)
            continue;
        if(ch->getIfcSrc() != ifcLoc)
            continue;
        ch->markDeadChannel();
        notifyDeadChannel(chId);
    }
}

void NewChannelScheculer::notifyFreeCell(appInt8 chId) {
}

void NewChannelScheculer::close() {
    for(auto i = 0; i < BASIC_CHANNEL_HANDLER_CHANNEL_COUNT; i++){
        if(channelHandlers[i]){
            LOGI("Initiating Shutting");
            channelHandlers[i]->initShutdown();
        }
    }
    for(auto i = 0; i < BASIC_CHANNEL_HANDLER_CHANNEL_COUNT; i++){
        if(channelHandlers[i]){
            LOGI("Shutting Down channel handler")
            channelHandlers[i]->shutDown();
            timeoutProducer().detach(channelHandlers[i]);
        }
    }
    waitToClose = TRUE;
    waitForToken.notify();
    waitForNewPacket.notify();
}

void NewChannelScheculer::readyToSend(appInt8 chId, appInt ready) {
    waitForToken.notify();
}

void NewChannelScheculer::addChannel(appInt8 ifcLoc, appInt8 ifcRem) {
    auto ifcMon = parent->getInterfaceMontor();
    APP_ASSERT(ifcRem != 0 && ifcRem < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT && remoteAddresses[ifcRem]);
    APP_ASSERT(ifcLoc != 0 && ifcLoc < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT && ((ifcMon && (*ifcMon)[ifcLoc]) || getInterfaceSender()[ifcLoc]));
    auto remote = remoteAddresses[ifcRem];
    channelHandler::ChannelHandler *ch;
    appChar *channelHandler = (appChar *)getenv("CHANNEL_HANDLER");
    if(channelHandler and strcmp((const char *)"BASIC", (const char *)channelHandler) == 0)
        ch = new channelHandler::BasicChannelHandler(this, ifcLoc, *remote, ifcLoc, ifcRem);//(this, *local, *remote);
    else if(channelHandler and strcmp((const char *)"SACK", (const char *)channelHandler) == 0)
        ch = new channelHandler::SackNewRenoChannelHandler(this, ifcLoc, *remote, ifcLoc, ifcRem);
    else
        ch = new channelHandler::NewChannelHandler(this, ifcLoc, *remote, ifcLoc, ifcRem);//(this, *local, *remote);
    APP_ASSERT(ch);
    appInt8 channelId = ch->id();//(ifcLoc << 4) | (ifcRem&0x0f); // exactly as the packet header says :).
    if(channelHandlers[channelId]){
        delete ch;
        return;
    }
    ch->startUp();
    LOGD("Adding new channel");
    channelHandlers[channelId] = ch;
    timeoutProducer().attach(ch);
}

void NewChannelScheculer::run() {
    while(1){
        waitForToken.wait();
        if(waitToClose)
            break;
        schedule();
    }
}

void NewChannelScheculer::interfaceAdded(appInt8 ifcLoc) {
    auto ifcMon = parent->getInterfaceMontor();
    if(!(ifcLoc != 0 && ifcLoc < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT) && ((ifcMon && (*ifcMon)[ifcLoc]) || getInterfaceSender()[ifcLoc])){
        return;
    }
    for(appInt8 x = 1; x < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT; x++){
        if(remoteAddresses[x] == NULL){
            continue;
        }
        addChannel(ifcLoc, x);
    }
    LOGE("ifc added: %d", ifcLoc)
}

void NewChannelScheculer::interfaceRemoved(appInt8 ifcLoc) {
    for(appInt8 ifcRem = 0; ifcRem < BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT; ifcRem++){
        appInt8 chId = (ifcLoc << 4) | (ifcRem&0x0f);
        auto ch = channelHandlers[chId];
        if(!ch)
            continue;
        if(ch->getIfcSrc() != ifcLoc)
            continue;
        ch->markDeadChannel();
        notifyDeadChannel(chId);
    }
    LOGE("ifc removed: %d", ifcLoc)
    auto pkt = getPacketPool().getNewPacketWithData();
    pkt->header.flag = FLAG_IFC|FLAG_DAT;
    pkt->header.fingerPrint = getFingerPrint();
    auto pktHdr = GET_OPTIONAL_PACKET_HEADER_TYPE_IP_CHNG_ADDR;
    pktHdr->ifcId = ifcLoc;
    pktHdr->removed = 1;
    pkt->optHeaders = pktHdr;
    undeliveredPacketQueue.addToQueue(pkt);
    waitForNewPacket.notify();
}

void NewChannelScheculer::notifyDeadChannel(appInt8 chId) {
    deadChannelLock.lock();
    auto ch = channelHandlers[chId];
    if(!ch){
        deadChannelLock.unlock();
        return;
    }

    auto pktLL = ch->getUndeliveredPackets();
    auto cnt = undeliveredPacketQueue.addLLToQueue(pktLL);
    readyToSendLock.lock();
    channelHandlers[chId] = NULL;
    readyToSendLock.unlock();
    deadChannelLock.unlock();
    while(cnt){
        waitForNewPacket.notify();
        cnt--;
    }
    timeoutProducer().detach(ch);
    delete ch;
}

void NewChannelScheculer::schedule() {
    auto startedNextChildWith = nextChid;
    while(1){
        waitForNewPacket.wait();
        if(waitToClose)
            break;
        readyToSendLock.lock();
        if(!channelHandlers[nextChid] or !channelHandlers[nextChid]->haveCell()){
            nextChid++;
            if(nextChid == BASIC_CHANNEL_HANDLER_CHANNEL_COUNT)
                nextChid=1;
            readyToSendLock.unlock();
            waitForNewPacket.notify();
            if(nextChid == startedNextChildWith){
                break;
            }
            continue;
        }
        Packet *pkt = NULL;
        if(controlPkt){
            controlPktLock.lock();
            pkt = controlPkt;
            controlPkt = NULL;
            controlPktLock.unlock();
        }
        if(!pkt)
            pkt = undeliveredPacketQueue.getFromQueue();
        if(!pkt)
            pkt = outgoingPacketQueue.getFromQueue();
        APP_ASSERT(pkt)
//                break;
        channelHandlers[nextChid]->sendPacket(pkt);
//        }
        readyToSendLock.unlock();
    }
//    for(; nextChid < BASIC_CHANNEL_HANDLER_CHANNEL_COUNT; nextChid++){
//        if(!channelHandlers[nextChid])
//            continue;
//        Packet *pkt = NULL;
//        while(channelHandlers[nextChid]->haveCell()){
//            pkt = undeliveredPacketQueue.getFromQueue();
//            if(!pkt)
//                pkt = outgoingPacketQueue.getFromQueue();
//            if(!pkt)
//                break;
//            channelHandlers[nextChid]->sendPacket(pkt);
//        }
//        if(!pkt)
//            break;
//    }
//    if(nextChid == BASIC_CHANNEL_HANDLER_CHANNEL_COUNT)
//        nextChid=1;
}

} /* namespace util */
