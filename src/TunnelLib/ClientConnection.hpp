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
 * ClientConnection.hpp
 *
 *  Created on: 07-Aug-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_CLIENTCONNECTION_HPP_
#define SRC_TUNNELLIB_CLIENTCONNECTION_HPP_

#include "Connection.hpp"
#include <list>
#include <appThread.h>
#include "multiplexer.hpp"
#include "InterfaceScheduler.h"
#include "../../util/ConditonalWait.hpp"
#include "PacketEventHandler.h"
#include <mutex>

class ClientConnection: public BaseReliableObj {
public:

	ClientConnection(appByte *remoteIp, appInt remotePort);
	virtual ~ClientConnection();
	virtual appSInt sendPacketTo(appInt id, Packet *pkt,
			struct sockaddr_in *dest_addr, socklen_t addrlen);
    virtual appSInt recvPacketFrom(Packet *pkt);
	virtual inline appInt timeoutEvent(appTs time);

	virtual void handShake1(); //send syn
	virtual void handShake2(Packet *rcvPkt, sockaddr_in &src_addr); //recv syn|ack
	void startClientInSideThread();
	appStatus startClient();
	void waitToJoin();
//	void evTimerExpired();
	static void *startClientInsideThread(void *data, appThreadInfoId tid);
//	static void evTimerExpired(EV_P_  ev_timer *w, int revents);

	ARQ::Streamer *addNewFlow();
	virtual appSInt sendData(appInt16 flowId, appByte *data, appInt dataLen);
	virtual appSInt readData(appInt16 flowId, appByte *data, appInt dataLen);
	virtual appStatus closeFlow(appInt16 flowId);
	virtual inline TimeOutProducer &timeoutProducer(void) {return timeoutprod;}
	virtual inline PendingAcks &pendingAcks(void) {return pendAck;}
	void getOption(APP_TYPE::APP_GET_OPTION optType, void *optionValue, appInt optionValueLen, void *returnValue = NULL, appInt returnValueLen = 0);
	inline virtual appSInt recvAck(appInt16 flowId, appInt16 flowFeqNo) {assert(connectedMux); return connectedMux->recvAck(flowId, flowFeqNo);};
	void close();
	virtual UTIL::WorkerThread *getWorker(WorkerType type);
	virtual inline appInt16 getFingerPrint(void) { return fingerPrint; }
//=========
private:
	void close1();
	void close2();
	appInt16 getNextFlowId();
	void setupIterface();
	appByte *remoteIp;
	appInt remotePort;
	int socketFd;
	struct sockaddr_in localAddr, remoteAddr;
	socklen_t localAddrLen, remoteAddrLen;
//	struct ev_loop *loop;
	pthread_t threadId;
//	appBool threadRunning;
//	ev_timer timer;
	appInt16 fingerPrint;
	Multiplexer *connectedMux;
	appInt16 nextFlowId;
	appBool connected;
	appInt syncTries;
	appSInt64 syncTime;
	sem_t waitToConnect;
	PacketEventHandler *con;
	std::mutex threadSafetyUsr, threadSafetyCon, threadSafety;
	void *newEventCallBackData;
	InterfaceScheduler ifcSch;
	TimeOutProducer timeoutprod;
	PendingAcks pendAck;
	appInt8 primaryInterfaceId;
	appInt closeTry;
//	appBool closeInitiated;
	ConditionalWait waitToClose;
	appSInt64 closeTime;
    std::map<WorkerType, UTIL::WorkerThread *> workers;
    std::mutex getWorkerMutex;
};




#endif /* SRC_TUNNELLIB_CLIENTCONNECTION_HPP_ */
