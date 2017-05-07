/*
 * ServerConnection.cc
 *
 *  Created on: 07-Aug-2016
 *      Author: abhijit
 */

#include "ServerConnection.hpp"
#include "PacketPool.h"
appSInt ServerConnection::sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen) {
	connection->sendPacketTo(id, pkt, dest_addr, addrlen);
	return 0;
}

appSInt ServerConnection::recvPacketFrom(Packet *pkt){
	if(pkt->header.fingerPrint == EMPTY_FINGER_PRINT){
		if((pkt->header.flag & FLAG_SYN) and (validateNewClient && validateNewClient(this, pkt, pkt->src_addr))){
			Packet *newPkt = getPacketPool().getNewPacketWithData();
			generateFingerPrint(&newPkt->header.fingerPrint, pkt->src_addr);
			LOGD("genpkt: %d\n", newPkt->header.fingerPrint);
			newPkt->header.flag = FLAG_ACK|FLAG_SYN;
			ClientDetails *cl = new ClientDetails(newPkt->header.fingerPrint, NULL, this);
			pendingClients[newPkt->header.fingerPrint] = cl;
			newPkt->header.ifcsrc = primaryInterfaceId;
			newPkt->header.ifcdst = pkt->header.ifcsrc;
			newPkt->optHeaders = getIpHeaders(&newPkt->header.OHLen);
//			newPkt->freeAtInterFacesinding = TRUE;
			getInterfaceSender()[primaryInterfaceId]->sendPkt(newPkt, pkt->src_addr.sin_addr, pkt->src_addr.sin_port);
			getPacketPool().freePacket(newPkt);
		}
		getPacketPool().freePacket(pkt);
		return 0;
	}
	auto iter = clients.find(pkt->header.fingerPrint);
	ClientDetails *client;
	if(iter == clients.end())
	{
		iter = pendingClients.find(pkt->header.fingerPrint);
		if(iter == pendingClients.end()){
		    if(pkt->header.flag&FLAG_CFN){
		        Packet *npkt = getPacketPool().getNewPacket();
		        npkt->header.flag = FLAG_CFN|FLAG_ACK;
		        getInterfaceSender()[primaryInterfaceId]->sendPkt(npkt, pkt->src_addr.sin_addr, pkt->src_addr.sin_port);
		        getPacketPool().freePacket(npkt);
		    }
		    else
		        LOGI("Unknown request");
			getPacketPool().freePacket(pkt);
			return 1;
		}
		client = iter->second;
		pendingClients.erase(pkt->header.fingerPrint);
		clients[pkt->header.fingerPrint] = client;
		Multiplexer *mux = new Multiplexer(client);
		client->setMux(mux);
//		client->setupInterfaces(pkt->header.ifcsrc, pkt->src_addr);
	}
	else
		client = iter->second;
	if(!client->isValidFlow(pkt->header.flowId) and pkt->header.flowId)
	{
	    LOGD("adding new flow %d for client %d", pkt->header.flowId, client->getFingerPrint());
	    auto obj = client->addNewFlow(pkt, pkt->src_addr, pkt->dest_addr);
	    if(newFlowCB)
	        newFlowCB(this, pkt->header.fingerPrint, pkt->header.flowId, obj);
	}
	if(pkt->header.flag&FLAG_CFN){
	    closeClient(client);
	    Packet *npkt = getPacketPool().getNewPacket();
	    npkt->header.flag = FLAG_CFN|FLAG_ACK;
	    getInterfaceSender()[primaryInterfaceId]->sendPkt(npkt, pkt->src_addr.sin_addr, pkt->src_addr.sin_port);
	    getPacketPool().freePacket(npkt);
	    getPacketPool().freePacket(pkt);
	    return 0;
	}
	return client->recvPacketFrom(pkt);
////	    TODO do something with new flow in serverconnection
}


appSInt ServerConnection::readData(appInt16 fPrint, appInt16 flowId, appByte *data, appInt size){
	if(!hasKey(clients, fPrint))
		return -1;
	ClientDetails *child = clients[fPrint];
	return child->readData(flowId, data, size);
}

appSInt ServerConnection::sendData(appInt16 fPrint, appInt16 flowId, appByte *data, appInt size){
	if(!hasKey(clients, fPrint))
		return -1;
	ClientDetails *child = clients[fPrint];
	return child->sendData(flowId, data, size);
}

void ServerConnection::getOption(APP_TYPE::APP_GET_OPTION optType,
        void* optionValue, appInt optionValueLen, void* returnValue,
        appInt returnValueLen) {
    APP_ASSERT(optionValue && optionValueLen);
    switch(optType){
        case APP_TYPE::APP_GET_STREAM_FLOW_AND_FINGER_PRINT:
            {
                appInt16 fprint, flowId;
                appInt16 *vals;
                APP_ASSERT(optionValueLen == sizeof(appInt16)*2);
                vals = (appInt16*)optionValue;
                fprint = vals[0];
                flowId = vals[1];
                if(!hasKey(clients, fprint))
                    return;
                ClientDetails *cl= clients[fprint];
                cl->getOption(APP_TYPE::APP_GET_STREAM_FLOW, &flowId, sizeof(appInt16), returnValue, returnValueLen);
            }
            break;
        default:
            APP_ASSERT(0 && "not allowed");
    }
}

void ServerConnection::closeClient(ClientDetails* client) {
    //TODO
    if(!client)
        return;
    LOGI("Closing");
    if(hasKey(clients, client->getFingerPrint())){
        clients.erase(client->getFingerPrint());
    }
    client->close();
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

UTIL::WorkerThread* ServerConnection::getWorker(WorkerType type) {
    if(hasKey(workers, type))
        return workers[type];
    getWorkerMutex.lock();
    if(hasKey(workers, type))
        return workers[type];
    auto tmp = new UTIL::WorkerThread(true);
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
        auto sender = getInterfaceSender()[it];
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

void ServerConnection::setupIterface(){
    auto list = SearchMac::getIpMatrix();
    auto cnt = 0;
    if(list.size() > 0){
        for(auto it : list){
            it.localPort = htons(localPort);
            SendThroughInterface *tmp = new SendThroughInterface(&it);
            tmp->init();
            cnt ++;
            getInterfaceSender()[cnt] = tmp; //AS
        }
        return;
    }
    LOGE("Routing table doesn't contain defaut gateway. Falling back to conventional way.");
    auto ifcs = SearchMac::getInterfaceInfos();
    InterfaceInfo *tmp;
    for(auto it : ifcs){
        tmp = new InterfaceInfo();
        cnt++;
        *tmp = it;
        getInterfaceInfos()[cnt] = tmp;
        SendThroughInterface *sti = new SendThroughInterface((appString)it.ifname.c_str(), it.ip, (appInt)htons(localPort));
        sti->init();
        getInterfaceSender()[cnt] = sti;
    }
}

appStatus ServerConnection::startServer() {
//    ServerConnection *sCon = this;
//    appByte *buf = APP_PACK(sCon);
    connection = new PacketEventHandler(this);
    connection->startServer(localPort, listeningIp);
    setupIterface();
//    ev_init(&timer, evTimerExpired);
//    timer.repeat = 0.2; //100milisec
//    timer.data = this;
//    ev_timer_again(EV_DEFAULT_ &timer);
//    if(runInThreadGetTid(ServerConnection::startServerInsideThread, buf, FALSE, &this->threadId) != APP_SUCCESS)
//        APP_RETURN_FAILURE
//    this->threadRunning = TRUE;
    APP_RETURN_SUCCESS
}

void ServerConnection::waitToJoin(){
    connection->waitToFinishAllThreads();
//    if(threadRunning)
//        pthread_join(threadId, NULL);
}


void *ServerConnection::startServerInsideThread(void *data, appThreadInfoId tid){
	ServerConnection *sCon;
	APP_UNPACK((appByte *)data, sCon);
    sCon->threadRunning = TRUE;
    ev_run(EV_DEFAULT_ 0);
	return NULL;
}
appInt ServerConnection::timeoutEvent(appTs time){
	timeoutProd.timeoutEvent(time);
	return 0;
}
void ServerConnection::evTimerExpired(EV_P_ ev_timer *w, int revents){
	ServerConnection *sCon = (ServerConnection *)w->data;
	sCon->timeoutEvent(sCon->getTime());
	ev_timer_again(loop, w);
}

appStatus ServerConnection::closeFlow(appInt16 fp, appInt16 flowId){
	if(!hasKey(clients, fp))
			return APP_FAILURE;
	ClientDetails *child = clients[fp];
	return child->closeFlow(flowId);

}
