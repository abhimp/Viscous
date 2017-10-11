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
 * ClientConnection.hh
 *
 *  Created on: 07-Aug-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_CLIENTCONNECTION_HPP_
#define SRC_TUNNELLIB_CLIENTCONNECTION_HPP_

#include <appThread.h>
#include <common.h>
#include <semaphore.h>
#include <stddef.h>
#include <unistd.h>
#include <cassert>
#include <map>
#include <mutex>

#include "../util/ConditonalWait.hh"
#include "../util/ThreadPool.hh"
#include "appTypes.h"
#include "ChannelScheduler/NewChannelScheculer.hh"
#include "ChannelScheduler/InterfaceScheduler.hh"
#include "CommonHeaders.hh"
#include "multiplexer.hh"
#include "PendingAcks.hh"

class PacketEventHandler;
class ClientConnection;

class ClientConnectionThread: public util::AppThread{
public:
    ClientConnectionThread(ClientConnection *cc) : stop(FALSE), cc(cc){}
    ~ClientConnectionThread();
    void run();
    appSInt recvPacket(Packet *pkt);
private:
    util::AppLLQueue<Packet> recvPktQueue;
    util::AppSemaphore recvPktQueueLock;
    util::AppSemaphore waitToStop;
    appBool stop;
    ClientConnection *cc;
};

class ClientConnection: public BaseReliableObj {
public:

    ClientConnection(appByte *remoteIp, appInt remotePort);
    virtual ~ClientConnection();
    virtual appSInt sendPacketTo(appInt id, Packet *pkt);
    virtual appSInt recvPacketFrom(Packet *pkt, RecvSendFlags &flags);
    virtual appSInt recvPacketFromQueue(Packet *pkt, RecvSendFlags &flags);
    virtual inline appInt timeoutEvent(appTs time);

    virtual void handShake1(); //send syn
    virtual void handShake2(Packet *rcvPkt, sockaddr_in &src_addr); //recv syn|ack
//    void startClientInSideThread();
    appStatus startClient();
    void waitToJoin();
//    static void *startClientInsideThread(void *data, appThreadInfoId tid);

    appInt32 addNewFlow();
    virtual appSInt sendData(appInt32 flowId, appByte *data, appInt dataLen);
    virtual appSInt readData(appInt32 flowId, appByte *data, appInt dataLen);
    appStatus closeFlow(appInt32 flowId);
    virtual inline TimeOutProducer &timeoutProducer(void) {return timeoutprod;}
    virtual inline PendingAcks &pendingAcks(void) {return pendAck;}
//    void getOption(APP_TYPE::APP_GET_OPTION optType, void *optionValue, appInt optionValueLen, void *returnValue = NULL, appInt returnValueLen = 0);
    inline virtual appSInt recvAck(appFlowIdType flowId, appInt16 flowFeqNo) { APP_ASSERT(connectedMux); return connectedMux->recvAck(flowId, flowFeqNo);};
    void close();
    virtual util::WorkerThread *getWorker(WorkerType type);
    virtual inline appInt16 getFingerPrint(void) { return fingerPrint; }
    virtual inline appBool isServer(void) {return FALSE;}
    virtual inline appBool isClient(void) {return TRUE;}
    virtual inline PacketReadHeader *getReceiverStatus(){ APP_ASSERT(connectedMux); return connectedMux->getReceiverStatus();}
    virtual inline InterfaceMonitor *getInterfaceMontor(){return ifcMon;}
    virtual inline appSInt recvProcPacket(Packet *pkt);
    virtual appSInt recvProcPacketFromQueue(Packet *pkt, RecvSendFlags &flags);

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
//    struct ev_loop *loop;
    pthread_t threadId;
//    appBool threadRunning;
//    ev_timer timer;
    appInt16 fingerPrint;
    Multiplexer *connectedMux;
    appInt16 nextFlowId;
    appBool connected;
    appInt syncTries;
    appSInt64 syncTime;
    sem_t waitToConnect;
    PacketEventHandler *con;
    util::AppMutex threadSafetyUsr, threadSafetyCon, threadSafety;
    void *newEventCallBackData;
#ifdef NEW_CHANNEL_HANDLE
    scheduler::NewChannelScheculer ifcSch;
#else
    scheduler::InterfaceScheduler ifcSch;
#endif
    TimeOutProducer timeoutprod;
    PendingAcks pendAck;
    InterfaceMonitor *ifcMon;
//    appInt8 primaryInterfaceId;
    appInt closeTry;
//    appBool closeInitiated;
    util::ConditionalWait waitToClose;
    appSInt64 closeTime;
    std::map<WorkerType, util::WorkerThread *> workers;
    util::AppMutex getWorkerMutex;
    ClientConnectionThread cct;
    virtual appStatus closeFlow(appFlowIdType flowId){APP_ASSERT(0 and "not allowed here"); return APP_SUCCESS;}
    virtual appSInt sendData(appFlowIdType flowId, appByte *data, appInt dataLen){APP_ASSERT(0 and "not allowed here"); return APP_SUCCESS;};
    virtual appSInt readData(appFlowIdType flowId, appByte *data, appInt dataLen){APP_ASSERT(0 and "not allowed here"); return APP_SUCCESS;};
};




#endif /* SRC_TUNNELLIB_CLIENTCONNECTION_HPP_ */
