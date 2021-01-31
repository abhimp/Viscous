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
 * ServerConnection.cc
 *
 *  Created on: 07-Aug-2016
 *      Author: abhijit
 */

#include "ServerConnection.hh"
#include "PacketPool.hh"
#include "../util/AppLLQueue.hh"
#include "../util/CircularBuffer.hh"
#include "../util/ConditonalWait.hh"
#include "ChannelHandler/ChannelHandler.hh"
#include "CommonHeaders.hh"
#include "Packet.h"

namespace server{

ServerConnection::ServerConnection(appInt port, appByte* ip): BaseReliableObj(NULL)
        , listeningIp(ip)
        , localPort(port)
        , threadRunning(FALSE)
        , threadId(0)
        , connection(NULL)
        , newDataCallBackData(NULL)
//        , primaryInterfaceId_(1)
        , validateNewClient(NULL)
        , newFlowCB(NULL)
        , maxWaiting(0)
        , ifcMon(NULL)
{
}

ServerConnection::~ServerConnection() {
    for(auto it:workers){
        it.second->stop();
    }
    if(connection){
        delete connection;
        connection = NULL;
    }
}

appSInt ServerConnection::sendPacketTo(appInt id, Packet *pkt) {
    connection->sendPacketTo(id, pkt);
    return 0;
}

appSInt ServerConnection::recvPacketFrom(Packet *pkt, RecvSendFlags &flags){
    if(pkt->header.fingerPrint == EMPTY_FINGER_PRINT){
        if(!(pkt->header.flag&FLAG_SYN)){
            getPacketPool().freePacket(pkt);
            return 0;
        }

        auto nonce = getNonce(pkt);
        if(nonce == 0){
            getPacketPool().freePacket(pkt);
            return 0;
        }

        appBool pendingClient = hasKey(nonce2FingerPrint, nonce);
        if(!pendingClient){
            appInt16 fingerPrint;

            generateFingerPrint(&fingerPrint, pkt->src_addr);
            nonce2FingerPrint[nonce] = fingerPrint;
            fingerPrint2Nonce[fingerPrint] = nonce;

            Packet *newPkt = getPacketPool().getNewPacketWithData();
            newPkt->header.fingerPrint = fingerPrint;
            LOGD("genpkt: %d\n", newPkt->header.fingerPrint);
            newPkt->header.flag = FLAG_ACK|FLAG_SYN;
            Client4Server *cl = new Client4Server(newPkt->header.fingerPrint, NULL, this);
            std::shared_ptr<Client4Server> x(cl);
            pendingClients[newPkt->header.fingerPrint] = x;
            auto pIfcId = ifcMon->getSuitableInterfaceId(pkt->src_addr.sin_addr.s_addr);
            newPkt->header.ifcsrc = pIfcId;
            newPkt->header.ifcdst = pkt->header.ifcsrc;
            newPkt->optHeaders = getIpHeaders(&newPkt->header.OHLen);
            ifcMon->get(pIfcId)->sendPkt(newPkt, pkt->src_addr.sin_addr, pkt->src_addr.sin_port);
            getPacketPool().freePacket(newPkt);
            getPacketPool().freePacket(pkt);
            return 0;
        }
        else if((pkt->header.flag & FLAG_SYN) and pendingClient){
            Packet *newPkt = getPacketPool().getNewPacketWithData();
            newPkt->header.fingerPrint = nonce2FingerPrint[nonce];
            newPkt->header.flag = FLAG_ACK|FLAG_SYN;
//            Client4Server *cl = new Client4Server(newPkt->header.fingerPrint, NULL, this);
//            std::shared_ptr<Client4Server> x(cl);
//            pendingClients[newPkt->header.fingerPrint] = x;
            auto pIfcId = ifcMon->getPrimaryInterfaceId();
            newPkt->header.ifcsrc = pIfcId;
            newPkt->header.ifcdst = pkt->header.ifcsrc;
            newPkt->optHeaders = getIpHeaders(&newPkt->header.OHLen);
            ifcMon->get(pIfcId)->sendPkt(newPkt, pkt->src_addr.sin_addr, pkt->src_addr.sin_port);
            getPacketPool().freePacket(newPkt);
        }

        getPacketPool().freePacket(pkt);
        return 0;
    }
    auto iter = clients.find(pkt->header.fingerPrint);
    std::shared_ptr<Client4Server> client;

    if(iter == clients.end())
    {
        iter = pendingClients.find(pkt->header.fingerPrint);
        if(iter == pendingClients.end()){
            if(pkt->header.flag&FLAG_CFN){
                Packet *npkt = getPacketPool().getNewPacket();
                npkt->header.flag = FLAG_CFN|FLAG_ACK;
                auto pIfcId = ifcMon->getPrimaryInterfaceId();
                ifcMon->get(pIfcId)->sendPkt(npkt, pkt->src_addr.sin_addr, pkt->src_addr.sin_port);
                getPacketPool().freePacket(npkt);
            }
            else
                LOGI("Unknown request");
            getPacketPool().freePacket(pkt);
            return 1;
        }
        client = iter->second;
        pendingClients.erase(pkt->header.fingerPrint);
//        appInt64 nonce = fingerPrint2Nonce[pkt->header.fingerPrint];
//        fingerPrint2Nonce.erase(pkt->head er.fingerPrint);
//        nonce2FingerPrint.erase(nonce);

        clients[pkt->header.fingerPrint] = client;
        Multiplexer *mux = new Multiplexer(client.get(), pkt->header.fingerPrint);
        client->setMux(mux);
        client->start();
    }
    else
        client = iter->second;

    pkt->newFlow = FALSE;
    RecvSendFlags flag = DEFALT_RECVSENDFLAG_VALUE;
    auto ret = client->recvPacketFrom(pkt, flag);
    return ret;
////        TODO do something with new flow in serverconnection
}


appSInt ServerConnection::readData(appInt32 flowId, appByte *data, appInt size){
    appFlowIdType flId;
    flId.clientFlowId = flowId;
    if(!hasKey(clients, flId.fingerPrint))
        return -1;
    auto child = clients[flId.fingerPrint];
    return child->readData(flId, data, size);
}

appSInt ServerConnection::sendData(appInt32 flowId, appByte *data, appInt size){
    appFlowIdType flId;
    flId.clientFlowId = flowId;
    if(!hasKey(clients, flId.fingerPrint))
        return -1;
    auto child = clients[flId.fingerPrint];
    return child->sendData(flId, data, size);
}

util::WorkerThread* ServerConnection::getWorker(WorkerType type) {
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

PacketIpHeader* ServerConnection::getIpHeaders(appInt8 *ohcnt) {
    PacketIpHeader *list, *tmp;
    list = NULL;
    tmp = NULL;
    appInt8 cnt = 0;
    for(auto it = 0 ; it < INTERFACE_SENDER_CNT; it++){
        auto sender = ifcMon->get(it);
        if(sender == NULL)
            continue;
        tmp = GET_OPTIONAL_PACKET_HEADER_TYPE_IP_ADDR;
        tmp->ifcId = it;
        tmp->ip = ntohl(sender->getLocalIpNetwork().s_addr);
        tmp->next = list;
        list = tmp;
        cnt++;
    }
    *ohcnt = cnt;
    return list;
}

void ServerConnection::newFlowNoticfication(appInt16 fngrPrnt) {
    acceptLock.lock();
    if(hasKey(pendingFlowClientId, fngrPrnt)){
        auto x = pendingFlowClientId[fngrPrnt];
        x++;
        pendingFlowClientId[fngrPrnt] = x;
    }
    else{
        pendingFlowClientId[fngrPrnt] = 1;
    }
    acceptSem.notify();
    acceptLock.unlock();
}

appInt64 ServerConnection::getNonce(Packet *pkt){
    auto optHeader = pkt->optHeaders;
    appInt64 nonce = 0;
    for(;optHeader;optHeader = optHeader->next){
        if(optHeader->type == OPTIONAL_PACKET_HEADER_TYPE_NONCE_HEADER){
            auto nonceHdr = (PacketNonceHeader *) optHeader;
            nonce = nonceHdr->nonce;
            break;
        }
    }
    return nonce;
}

appInt32 ServerConnection::acceptFlow() {
    appInt32 fingerPrint;
    acceptSem.wait();
    acceptLock.lock();
    auto it = pendingFlowClientId.begin();
    auto fngrPrnt = it->first;
    auto val = it->second;
    val--;
    if(val == 0)
        pendingFlowClientId.erase(fngrPrnt);
    else
        pendingFlowClientId[fngrPrnt] = val;
    acceptLock.unlock();
    auto client = clients[fngrPrnt];
    fingerPrint = client->acceptFlow();
    return fingerPrint;
}

void ServerConnection::listen(appInt count) {
    maxWaiting = count;
}

void ServerConnection::setupIterface(){
    ifcMon = new InterfaceMonitor(localPort);
    ifcMon->start();
    if(ifcMon->getPrimaryInterfaceId() == 0){
        LOGE("No interface have proper routing. exiting.\n");
        exit(100);
    }
}

appStatus ServerConnection::startServer() {
    connection = new PacketEventHandler(this);
    connection->startServer(localPort, listeningIp);
    setupIterface();
    APP_RETURN_SUCCESS
}

void ServerConnection::waitToJoin(){
    connection->waitToFinishAllThreads();
}

appInt ServerConnection::timeoutEvent(appTs time){
    timeoutProd.timeoutEvent(time);
    if(time.getMili() - lastTimeout.getMili() > 5000){
        lastTimeout = time;
        std::set<appInt16> toBeDeleted;
        for(auto cl : clients){
            auto client = cl.second;
            if(client->isClosed() and (time.getMili() - client->getLastUsed().getMili() > 5000)){
                toBeDeleted.insert(cl.first);
            }
        }
        for(auto x : toBeDeleted){
            if(hasKey(fingerPrint2Nonce, x)){
                appInt64 nonce = fingerPrint2Nonce[x];
                fingerPrint2Nonce.erase(x);
                nonce2FingerPrint.erase(nonce);
            }
            clients.erase(x);
            LOGE("Closing %d", x);
        }
    }
    return 0;
}

appStatus ServerConnection::closeFlow(appInt32 fp){
    appFlowIdType flowId;
    flowId.clientFlowId = fp;
    if(!hasKey(clients, flowId.fingerPrint))
            return APP_FAILURE;
    auto child = clients[flowId.fingerPrint];
    return child->closeFlow(flowId);
}

} //namespace server
