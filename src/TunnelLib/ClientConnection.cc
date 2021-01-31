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
 * ClientConnection.cc
 *
 *  Created on: 04-Aug-2016
 *      Author: abhijit
 */
#include "ClientConnection.hh"

#include "PacketPool.hh"
#include "PacketEventHandler.hh"

union TempClientNonce{
    struct{
        appInt32 serverIP; //
        appInt16 serverPort;
        appInt16 clientPort;
    };
    appInt64 nonce;
};

ClientConnection::ClientConnection(appByte *remoteIp, appInt remotePort) : BaseReliableObj(NULL)
        , remoteIp(remoteIp)
        , remotePort(remotePort)
        , socketFd(0)
        , localAddrLen(sizeof(localAddr))
        , remoteAddrLen(sizeof(remoteAddr))
        , threadId(0)
        , fingerPrint(EMPTY_FINGER_PRINT)
        , connectedMux(NULL)
        , nextFlowId(0)
        , connected(FALSE)
        , syncTries(0)
        , syncTime(0)
        , con(NULL)
        , newEventCallBackData(NULL)
        , ifcSch(this)
        , ifcMon(NULL)
//        , primaryInterfaceId(1)
        , closeTry(0)
        , closeTime(0)
        , cct(this)
{
    sem_init(&waitToConnect, 0, 0);
    connectedMux = new Multiplexer(this, fingerPrint);
    std::memset(&localAddr, 0, sizeof(localAddr));
    std::memset(&remoteAddr, 0, sizeof(remoteAddr));
}

void ClientConnection::handShake1(){
    Packet *pkt = getPacketPool().getNewPacketWithData();
    pkt->header.fingerPrint = EMPTY_FINGER_PRINT;
    pkt->header.flag = FLAG_SYN;
//    pkt->header.ifcsrc = ifcMon->getPrimaryInterfaceId();
    pkt->header.ifcsrc = ifcMon->getSuitableInterfaceId(remoteAddr.sin_addr.s_addr);

    TempClientNonce nonce = {remoteAddr.sin_addr.s_addr, remoteAddr.sin_port, localAddr.sin_port};
    auto nonceHeader = GET_OPTIONAL_PACKET_HEADER_TYPE_NONCE;
    nonceHeader->nonce = nonce.nonce;
    nonceHeader->next = pkt->optHeaders;
    pkt->optHeaders = nonceHeader;

    (*ifcMon)[pkt->header.ifcsrc]->sendPkt(pkt, remoteAddr.sin_addr, remoteAddr.sin_port);
    syncTries ++;
    syncTime = getTime().getMicro();
    getPacketPool().freePacket(pkt);
//    delete nonceHeader;
    LOGI("Trying to connect: %d\n", syncTries);
}

void ClientConnection::handShake2(Packet *rcvPkt, sockaddr_in &srcAdr){
    Packet *pkt = getPacketPool().getNewPacketWithData();
    fingerPrint = rcvPkt->header.fingerPrint;
    connectedMux->setFingerPrint(fingerPrint);
    pkt->header.fingerPrint = fingerPrint;
    pkt->header.flag = FLAG_ACK;
//    auto primaryInterfaceId = ifcMon->getPrimaryInterfaceId();
    auto primaryInterfaceId = ifcMon->getSuitableInterfaceId(remoteAddr.sin_addr.s_addr);
    pkt->header.ifcsrc = primaryInterfaceId;
    pkt->header.ifcdst = rcvPkt->header.ifcsrc;
    APP_ASSERT(rcvPkt->header.ifcsrc);
    (*ifcMon)[primaryInterfaceId]->sendPkt(pkt, remoteAddr.sin_addr, remoteAddr.sin_port);
//    TODO Make start client blocking. No need to send notification.
    auto oh = rcvPkt->optHeaders;
    while(oh){
        if(oh->type != OPTIONAL_PACKET_HEADER_TYPE_IP_ADDR)
            continue;
        auto ipHdr = (PacketIpHeader *)oh;
        sockaddr_in addr;
        addr.sin_addr.s_addr = htonl(ipHdr->ip);
        addr.sin_port = srcAdr.sin_port;
        RemoteAddr remAdr(addr);
        ifcSch.addRemote(ipHdr->ifcId, &remAdr);
        oh = oh->next;
    }
    connected = TRUE;
    syncTries = 0;
    getPacketPool().freePacket(rcvPkt);
    getPacketPool().freePacket(pkt);
    sem_post(&waitToConnect);
}

ClientConnection::~ClientConnection() {
    close();
    delete connectedMux;
    delete con;
    auto x = getInterfaceSender();
    for(auto i = 0; i < INTERFACE_SENDER_CNT; i++){
        if(x[i]) delete x[i];
    }
    for(auto it:workers){
        it.second->stop();
    }
}

appSInt ClientConnection::sendPacketTo(appInt id, Packet *pkt) {
    pkt->header.fingerPrint = fingerPrint;
    return ifcSch.sendPacketTo(id, pkt);
}

appSInt ClientConnection::recvPacketFrom(Packet *pkt, RecvSendFlags &flags) {
    auto ret = cct.recvPacket(pkt);
    if(ret < 0)
        getPacketPool().freePacket(pkt);
    return 0;
}

inline appSInt ClientConnection::recvProcPacket(Packet* pkt) {
    APP_ASSERT(pkt->processed);
    return cct.recvPacket(pkt);
}

appSInt ClientConnection::recvProcPacketFromQueue(Packet* pkt,
        RecvSendFlags& flags) {
    threadSafetyCon.lock();
    appSInt ret = this->connectedMux->recvPacketFrom(pkt, flags);
    if(ret){
        getPacketPool().freePacket(pkt);
    }
    threadSafetyCon.unlock();
    return ret;
}

appSInt ClientConnection::recvPacketFromQueue(Packet *pkt, RecvSendFlags &flags) {
    APP_ASSERT((pkt->header.ifcdst && pkt->header.ifcsrc)||((pkt->header.flag&(FLAG_CFN|FLAG_ACK))==(FLAG_CFN|FLAG_ACK)));
    if((pkt->header.flag & (FLAG_SYN | FLAG_ACK)) == (FLAG_SYN | FLAG_ACK)){ //i.e. i am waiting for sync and server returns me a ack and syn
        handShake2(pkt, pkt->src_addr);
        return 0;
    }
    if((pkt->header.flag & (FLAG_CFN | FLAG_ACK)) == (FLAG_CFN | FLAG_ACK)){
        close2();
        getPacketPool().freePacket(pkt);
        return 0;
    }
    threadSafetyCon.lock();
    ifcSch.recvPacketFrom(pkt, flags);
    appSInt ret = this->connectedMux->recvPacketFrom(pkt, flags);
    if(ret){
        getPacketPool().freePacket(pkt);
    }
    threadSafetyCon.unlock();
    return ret;
}

//void ClientConnection::getOption(APP_TYPE::APP_GET_OPTION optType, void* optionValue,
//        appInt optionValueLen, void* returnValue, appInt returnValueLen) {
//    switch(optType){
//        case APP_TYPE::APP_GET_STREAM_FLOW:
//            connectedMux->getOption(optType, optionValue, optionValueLen, returnValue, returnValueLen);
//            break;
//        default:
//            APP_ASSERT(0);
//    }
//}

void ClientConnection::close() {
    threadSafetyUsr.lock();
    if(closeTry or !connected)
        return;
    connectedMux->close();
    ifcSch.close();
    close1();
    waitToClose.wait();
    con->close();
    threadSafetyUsr.unlock();
}

void ClientConnection::close1() {
    closeTry ++;
    if(closeTry > 3){
        waitToClose.notify();
        return;
    }
    Packet *pkt = getPacketPool().getNewPacket();
    pkt->header.fingerPrint = fingerPrint;
    pkt->header.flag = FLAG_CFN;
    auto primaryInterfaceId = ifcMon->getSuitableInterfaceId(remoteAddr.sin_addr.s_addr);
    (*ifcMon)[primaryInterfaceId]->sendPkt(pkt, remoteAddr.sin_addr, remoteAddr.sin_port);
    getPacketPool().freePacket(pkt);
}

void ClientConnection::close2() {
    Packet *pkt = getPacketPool().getNewPacket();
    pkt->header.fingerPrint = fingerPrint;
    pkt->header.flag = FLAG_ACK;
    auto primaryInterfaceId = ifcMon->getSuitableInterfaceId(remoteAddr.sin_addr.s_addr);
    (*ifcMon)[primaryInterfaceId]->sendPkt(pkt, remoteAddr.sin_addr, remoteAddr.sin_port);
    getPacketPool().freePacket(pkt);
    waitToClose.notify();
}

void ClientConnection::setupIterface(){
    ifcMon = new InterfaceMonitor(con->getLocalPort());
    ifcMon->start();
    ifcMon->attach(&ifcSch);
    if(ifcMon->getPrimaryInterfaceId() == 0){
        LOGE("No interface have proper routing. exiting.\n");
        exit(100);
    }
}

appStatus ClientConnection::startClient() {
    cct.start();
    remoteAddr.sin_addr.s_addr = inet_addr((char *)remoteIp);
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(remotePort);
    remoteAddrLen = sizeof(remoteAddr);
    con = new PacketEventHandler(this);
    con->startClient(0);
    setupIterface();

    handShake1();
    sem_wait(&waitToConnect);
    if(!connected){
        APP_RETURN_FAILURE
    }
    APP_RETURN_SUCCESS
}

void ClientConnection::waitToJoin(){
    con->waitToFinishAllThreads();
}


appInt16 ClientConnection::getNextFlowId(){
    nextFlowId ++;
    return nextFlowId;
}

appInt32 ClientConnection::addNewFlow(){
    threadSafetyUsr.lock();
    threadSafetyCon.lock();
    appFlowIdType flowId;
    flowId.flowId = getNextFlowId();
    flowId.fingerPrint = fingerPrint;
    auto obj = connectedMux->getNewConnection(flowId);
    threadSafetyCon.unlock();
    threadSafetyUsr.unlock();
    return obj.clientFlowId;
}

appSInt ClientConnection::sendData(appInt32 flowId, appByte *data, appInt dataLen){
    appFlowIdType clflid;
    clflid.clientFlowId = flowId;
    appInt ret = connectedMux->sendData(clflid, data, dataLen);
    return ret;
}

appSInt ClientConnection::readData(appInt32 flowId, appByte *data, appInt dataLen){
    appFlowIdType clflid;
    clflid.clientFlowId = flowId;
    appInt ret = connectedMux->readData(clflid, data, dataLen);
    return ret;
}

appInt ClientConnection::timeoutEvent(appTs time) {
    if(!connected){
        if(time.getMicro() - syncTime > 1000000){
            if(syncTries && syncTries < 3) {
                handShake1();
                return 0;
            }
            else{
                LOGE("Some error");
                sem_post(&waitToConnect);
//                pthread_exit(NULL);
                return 0;
            }
        }
        return 1;
    }
    if(closeTry){
        if(time.getMicro() - syncTime > 1000000){
            close1();
        }
//        return 0;
    }
//    threadSafetyCon.lock();
    timeoutProducer().timeoutEvent(getTime());
//    threadSafetyCon.unlock();
    return 0;
}

appStatus ClientConnection::closeFlow(appInt32 flowId){
    appFlowIdType clflid;
    clflid.clientFlowId = flowId;
    return connectedMux->closeFlow(clflid);
}

util::WorkerThread* ClientConnection::getWorker(WorkerType type) {
    if(hasKey(workers, type))
        return workers[type];
    getWorkerMutex.lock();
    if(hasKey(workers, type))
        return workers[type];
    auto tmp = new util::WorkerThread(true);
    workers[type] = tmp;
    getWorkerMutex.unlock();
    return tmp;
}

void ClientConnectionThread::run() {
    while(1){
        recvPktQueueLock.wait();
        if(stop)
            break;
        auto pkt = recvPktQueue.getFromQueue();
        STOP_PACKET_PROFILER_CLOCK(pkt)
        if(!pkt)
            continue;

        RecvSendFlags flag = DEFALT_RECVSENDFLAG_VALUE;
        if(pkt->processed)
            cc->recvProcPacketFromQueue(pkt, flag);
        else
            cc->recvPacketFromQueue(pkt, flag);
    }
    waitToStop.notify();
}

appSInt ClientConnectionThread::recvPacket(Packet* pkt) {
    if(stop)
        return -1;
    START_PACKET_PROFILER_CLOCK(pkt)
    recvPktQueue.addToQueue(pkt);
    recvPktQueueLock.notify();
    return 0;
}

ClientConnectionThread::~ClientConnectionThread() {
    stop = TRUE;
    recvPktQueueLock.notify();
    waitToStop.wait();
}
