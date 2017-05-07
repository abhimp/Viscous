/*
 * ClientConnection.cc
 *
 *  Created on: 04-Aug-2016
 *      Author: abhijit
 */
#include "ClientConnection.hpp"
#include "PacketPool.h"

void ClientConnection::handShake1(){
	Packet *pkt = getPacketPool().getNewPacketWithData();
	pkt->header.fingerPrint = EMPTY_FINGER_PRINT;
	pkt->header.flag = FLAG_SYN;
	pkt->header.ifcsrc = primaryInterfaceId;
//	pkt->freeAtInterFacesinding = TRUE;
	getInterfaceSender()[primaryInterfaceId]->sendPkt(pkt, remoteAddr.sin_addr, remoteAddr.sin_port);
	syncTries ++;
	syncTime = getTime().getMicro();
	getPacketPool().freePacket(pkt);
	LOGI("Trying to connect: %d\n", syncTries);
}

void ClientConnection::handShake2(Packet *rcvPkt, sockaddr_in &srcAdr){
	Packet *pkt = getPacketPool().getNewPacketWithData();
	fingerPrint = rcvPkt->header.fingerPrint;
	pkt->header.fingerPrint = fingerPrint;
	pkt->header.flag = FLAG_ACK;
	pkt->header.ifcsrc = primaryInterfaceId;
	pkt->header.ifcdst = rcvPkt->header.ifcsrc;
//	pkt->freeAtInterFacesinding = TRUE;
	APP_ASSERT(rcvPkt->header.ifcsrc);
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
	getInterfaceSender()[primaryInterfaceId]->sendPkt(pkt, remoteAddr.sin_addr, remoteAddr.sin_port);
//	TODO Make start client blocking. No need to send notification.
	connected = TRUE;
	syncTries = 0;
	getPacketPool().freePacket(rcvPkt);
	getPacketPool().freePacket(pkt);
	sem_post(&waitToConnect);
}

ClientConnection::ClientConnection(appByte *remoteIp, appInt remotePort) :
		BaseReliableObj(NULL), remoteIp(remoteIp), remotePort(remotePort),
		socketFd(0), localAddrLen(sizeof(localAddr)), remoteAddrLen(sizeof(remoteAddr)),
		threadId(0),
//		threadRunning(FALSE),
		fingerPrint(EMPTY_FINGER_PRINT),
		connectedMux(NULL), nextFlowId(0), connected(FALSE), syncTries(0), syncTime(0),
		con(NULL), newEventCallBackData(NULL), ifcSch(this), primaryInterfaceId(1), closeTry(0),
		closeTime(0)
{
	sem_init(&waitToConnect, 0, 0);
//	pthread_mutex_init(&threadSafetyUsr, 0);
//	pthread_mutex_init(&threadSafetyCon, 0);
//	pthread_mutex_init(&threadSafety, 0);
	connectedMux = new Multiplexer(this);
	std::memset(&localAddr, 0, sizeof(localAddr));
	std::memset(&remoteAddr, 0, sizeof(remoteAddr));
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
//    delete ifcSch;
//    ifcSch
}

appSInt ClientConnection::sendPacketTo(appInt id, Packet *pkt,
        struct sockaddr_in *dest_addr, socklen_t addrlen) {
	pkt->header.fingerPrint = fingerPrint;
	if(dest_addr && addrlen)
		return ifcSch.sendPacketTo(id, pkt, dest_addr, addrlen);
	return ifcSch.sendPacketTo(id, pkt, (struct sockaddr_in *)&remoteAddr, remoteAddrLen);
}

appSInt ClientConnection::recvPacketFrom(Packet *pkt) {
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
//	pthread_mutex_lock(&threadSafetyCon);
	threadSafetyCon.lock();
	ifcSch.recvPacketFrom(pkt);
    appSInt ret = this->connectedMux->recvPacketFrom(pkt);
    if(ret){
        getPacketPool().freePacket(pkt);
    }
//    if(ret)
//        return ret;
//    pkt->reInitHeader();
//    getPacketPool().freePacket(pkt);
//	pthread_mutex_unlock(&threadSafetyCon);
    threadSafetyCon.unlock();
	return ret;
}

void ClientConnection::getOption(APP_TYPE::APP_GET_OPTION optType, void* optionValue,
        appInt optionValueLen, void* returnValue, appInt returnValueLen) {
    switch(optType){
        case APP_TYPE::APP_GET_STREAM_FLOW:
            connectedMux->getOption(optType, optionValue, optionValueLen, returnValue, returnValueLen);
            break;
        default:
            APP_ASSERT(0);
    }
}

void ClientConnection::close() {
//    pthread_mutex_lock(&threadSafetyUsr);
    threadSafetyUsr.lock();
    if(closeTry or !connected)
        return;
    connectedMux->close();
    ifcSch.close();
    close1();
    waitToClose.wait();
    con->close();
//    delete con;
//    pthread_mutex_unlock(&threadSafetyUsr);
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
//    pkt->freeAtInterFacesinding = TRUE;
    getInterfaceSender()[primaryInterfaceId]->sendPkt(pkt, remoteAddr.sin_addr, remoteAddr.sin_port);
    getPacketPool().freePacket(pkt);
}

void ClientConnection::close2() {
    Packet *pkt = getPacketPool().getNewPacket();
    pkt->header.fingerPrint = fingerPrint;
    pkt->header.flag = FLAG_ACK;
//    pkt->freeAtInterFacesinding = TRUE;
    getInterfaceSender()[primaryInterfaceId]->sendPkt(pkt, remoteAddr.sin_addr, remoteAddr.sin_port);
    getPacketPool().freePacket(pkt);
//    ev_break(EV_DEFAULT_ EVBREAK_ALL);
//    ev_loop_destroy(EV_DEFAULT);
    waitToClose.notify();
}

void ClientConnection::setupIterface(){
    auto list = SearchMac::getIpMatrix();
    auto cnt = 0;
    if(list.size() > 0){
        for(auto it : list){
            it.localPort = htons(con->getLocalPort());
            SendThroughInterface *tmp = new SendThroughInterface(&it);
            tmp->init();
            cnt ++;
            getInterfaceSender()[cnt] = tmp; //AS
        }
    return;
    }
    LOGE("Routing table doesn't contain defaut gateway. Falling back to conventional way.");
    exit(__LINE__);
    auto ifcs = SearchMac::getInterfaceInfos();
    InterfaceInfo *tmp;
    for(auto it : ifcs){
        cnt++;
        tmp = new InterfaceInfo();
        *tmp = it;
        getInterfaceInfos()[cnt] = tmp;
        SendThroughInterface *sti = new SendThroughInterface((appString)it.ifname.c_str(), it.ip, (appInt)htons(con->getLocalPort()));
        sti->init();
        getInterfaceSender()[cnt] = sti;
    }
}

appStatus ClientConnection::startClient() {
//    ClientConnection *cCon = this;
//    appByte *buf = APP_PACK(cCon);
    remoteAddr.sin_addr.s_addr = inet_addr((char *)remoteIp);
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(remotePort);
    remoteAddrLen = sizeof(remoteAddr);
    con = new PacketEventHandler(this);
    con->startClient(0);
    setupIterface();
//    ev_init(&timer, evTimerExpired);
//    timer.repeat = TIMER_GRANULITY_MS/1000.0; //200milisec
//    timer.data = this;
//    ev_timer_again(EV_DEFAULT_ &timer);

//    if(runInThreadGetTid(ClientConnection::startClientInsideThread, buf, FALSE, &this->threadId) != APP_SUCCESS)
//        APP_RETURN_FAILURE
    handShake1();
    sem_wait(&waitToConnect);
    if(!connected){
    	APP_RETURN_FAILURE
    }
    APP_RETURN_SUCCESS
}

void ClientConnection::waitToJoin(){
    con->waitToFinishAllThreads();
//    if(threadRunning)
//        pthread_join(threadId, NULL);
}

void *ClientConnection::startClientInsideThread(void *data, appThreadInfoId tid){
	ClientConnection *cCon;
	APP_UNPACK((appByte *)data, cCon);
//	cCon->threadRunning = TRUE;
	cCon->handShake1();
//	ev_run(EV_DEFAULT_ 0);
	return NULL;
}


appInt16 ClientConnection::getNextFlowId(){
	nextFlowId ++;
	return nextFlowId;
}

ARQ::Streamer *ClientConnection::addNewFlow(){
//	pthread_mutex_lock(&threadSafetyUsr);
//	pthread_mutex_lock(&threadSafetyCon);
	threadSafetyUsr.lock();
	threadSafetyCon.lock();
	appInt16 flowId = getNextFlowId();
	auto obj = connectedMux->getNewConnection(flowId);
//	pthread_mutex_unlock(&threadSafetyCon);
//	pthread_mutex_unlock(&threadSafetyUsr);
	threadSafetyCon.unlock();
	threadSafetyUsr.unlock();
	return obj;
}

appSInt ClientConnection::sendData(appInt16 flowId, appByte *data, appInt dataLen){
//	pthread_mutex_lock(&threadSafetyUsr);
	threadSafetyUsr.lock();
	appInt ret = connectedMux->sendData(flowId, data, dataLen);
//	pthread_mutex_unlock(&threadSafetyUsr);
	threadSafetyUsr.unlock();
	return ret;
}

appSInt ClientConnection::readData(appInt16 flowId, appByte *data, appInt dataLen){
//	pthread_mutex_lock(&threadSafetyUsr);
    threadSafetyUsr.lock();
	appInt ret = connectedMux->readData(flowId, data, dataLen);
//	pthread_mutex_unlock(&threadSafetyUsr);
	threadSafetyUsr.unlock();
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
                sem_post(&waitToConnect);
                pthread_exit(NULL);
                return 0;
            }
	    }
	    return 1;
	}
//	pthread_mutex_lock(&threadSafetyCon);
	threadSafetyCon.lock();
	timeoutProducer().timeoutEvent(getTime());
//	pthread_mutex_unlock(&threadSafetyCon);
	threadSafetyCon.unlock();
	return 0;
}

//void ClientConnection::evTimerExpired(EV_P_ ev_timer *w, int revents){
//	ClientConnection *cCon = (ClientConnection *)w->data;
//	cCon->evTimerExpired();
//	ev_timer_again(EV_DEFAULT_ w);
//}

appStatus ClientConnection::closeFlow(appInt16 flowId){
	return connectedMux->closeFlow(flowId);
}

UTIL::WorkerThread* ClientConnection::getWorker(WorkerType type) {
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
//appInt ClientConnection::timeoutEvent(appTs time)
//{
//	if(syncTries){
//		if(syncTries >= 3){
//			LOGE("Connection Failed\n");
//			exit(1);
//		}
//		handShake1();
//	}
//	if(closeTry and (time.getMicro()-closeTime) > (1000000) ){
//	    close1();
//	}
//	return connectedMux->timeoutEvent(time);
//}
