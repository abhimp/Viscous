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
 * ServerConnection.hh
 *
 *  Created on: 07-Aug-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_SERVERCONNECTION_HPP_
#define SRC_TUNNELLIB_SERVERCONNECTION_HPP_

#include "Connection.hh"
#include "multiplexer.hh"
#include <appThread.h>
#include <set>
#include <map>
#include <unordered_map>
#include <atomic>
#include <memory>

#include "PacketEventHandler.hh"
#include "ServerConnectionClientDetail.hh"


namespace server{

class ServerConnection : public BaseReliableObj{
public:
    typedef appBool (*validateNewClientCallBack)(ServerConnection *sCon, Packet *pkt, sockaddr_in &src_addr);
    typedef void (*newFlowCallBack)(ServerConnection *sCon, appInt16 fingerPrint, appInt16 newFlow, FlowHandler::Streamer *streamer);

    ServerConnection(appInt port, appByte *ip = (appByte *) "0.0.0.0");
    virtual ~ServerConnection();
    virtual appSInt sendPacketTo(appInt id, Packet *pkt);
    virtual appSInt recvPacketFrom(Packet *pkt, RecvSendFlags &flags);
    virtual appSInt sendData(appByte *data, appInt dataLen){return -1;};
    virtual appInt timeoutEvent(appTs time);

    virtual appSInt readData(appInt32 flowId, appByte *data, appInt size);
    virtual appSInt sendData(appInt32 flowId, appByte *data, appInt size);
    void setClientValidator(validateNewClientCallBack v){validateNewClient = v;}
    void setNewFlowCallBack(newFlowCallBack f){newFlowCB = f;}
    appStatus startServer();
    void waitToJoin();
    virtual appStatus closeFlow(appFlowIdType flowId){assert("NOT ALLOWED" && 0); return APP_FAILURE;}
    appStatus closeFlow(appInt32 flowId);

    virtual inline TimeOutProducer &timeoutProducer(void) {return timeoutProd;}

    virtual util::WorkerThread *getWorker(WorkerType type);
    virtual void listen(appInt count=5);
    virtual appInt32 acceptFlow();
    virtual inline appBool isServer(void) {return TRUE;}
    virtual inline appBool isClient(void) {return FALSE;}
    void newFlowNoticfication(appInt16 clientId);

    virtual inline InterfaceMonitor *getInterfaceMontor(){return ifcMon;}
private:

    PacketIpHeader *getIpHeaders(appInt8 *ohcnt);
    appByte *listeningIp;
    appInt localPort;
    appBool threadRunning;
    pthread_t threadId;

    PacketEventHandler *connection;
    void *newDataCallBackData;
    TimeOutProducer timeoutProd;

    virtual appSInt readData(appFlowIdType flowId, appByte *data, appInt size) {APP_ASSERT_MSG(0, "INVALID"); return 0;};
    virtual appSInt sendData(appFlowIdType flowId, appByte *data, appInt size) {APP_ASSERT_MSG(0, "INVALID"); return 0;};
    void setupIterface();
    appInt64 getNonce(Packet *pkt);
    validateNewClientCallBack validateNewClient;
    newFlowCallBack newFlowCB;
    std::map<WorkerType, util::WorkerThread *> workers;
    util::AppMutex getWorkerMutex;

    std::map<appInt16, std::shared_ptr<Client4Server> > clients; //Active Clients
    std::map<appInt64, appInt16> nonce2FingerPrint;
    std::map<appInt16, appInt64> fingerPrint2Nonce;
    std::map<appInt16, std::shared_ptr<Client4Server> > pendingClients; //accepted clients

    appInt maxWaiting;
    InterfaceMonitor *ifcMon;

    std::unordered_map<appInt16, appInt> pendingFlowClientId;
    util::AppSemaphore acceptSem;
    util::AppSemaphore newFlowNotificationSem;
    util::AppMutex acceptLock;
    appTs lastTimeout;
};

}// namespace server

#endif /* SRC_TUNNELLIB_SERVERCONNECTION_HPP_ */
