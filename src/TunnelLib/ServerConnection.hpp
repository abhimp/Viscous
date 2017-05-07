/*
 * ServerConnection.hpp
 *
 *  Created on: 07-Aug-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_SERVERCONNECTION_HPP_
#define SRC_TUNNELLIB_SERVERCONNECTION_HPP_

#include "Connection.hpp"
#include "multiplexer.hpp"
#include <appThread.h>
#include <set>
#include <map>
#include "ServerConnectionClientDetail.hpp"
#include "PacketEventHandler.h"


class ServerConnection : public BaseReliableObj{
public:
    typedef appBool (*validateNewClientCallBack)(ServerConnection *sCon, Packet *pkt, sockaddr_in &src_addr);
    typedef void (*newFlowCallBack)(ServerConnection *sCon, appInt16 fingerPrint, appInt16 newFlow, ARQ::Streamer *streamer);

	ServerConnection(appInt port, appByte *ip = (appByte *) "0.0.0.0") :
			BaseReliableObj(NULL), listeningIp(ip), localPort(port), threadRunning(FALSE),
			threadId(0), connection(NULL), clients(), pendingClients(),
			newDataCallBackData(NULL), primaryInterfaceId(1), validateNewClient(NULL), newFlowCB(NULL)
	{
		sem_init(&acceptNewFlow, 0, 0);
	}
	virtual ~ServerConnection();
	virtual appSInt sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen);
	virtual appSInt recvPacketFrom(Packet *pkt);
	virtual appSInt sendData(appByte *data, appInt dataLen){return -1;};
	virtual appInt timeoutEvent(appTs time);

	virtual appSInt readData(appInt16 fPrint, appInt16 flowId, appByte *data, appInt size);
	virtual appSInt sendData(appInt16 fPrint, appInt16 flowId, appByte *data, appInt size);
	void setClientValidator(validateNewClientCallBack v){validateNewClient = v;}
	void setNewFlowCallBack(newFlowCallBack f){newFlowCB = f;}
	appStatus startServer();
	void waitToJoin();
	virtual appStatus closeFlow(appInt16 flowId){assert("NOT ALLOWED" && 0); return APP_FAILURE;}
	appStatus closeFlow(appInt16 fp, appInt16 flowId);
	void closeClient(ClientDetails *client);

	static void *startServerInsideThread(void *data, appThreadInfoId tid);
	static void evTimerExpired(EV_P_ ev_timer *w, int revents);
	virtual inline TimeOutProducer &timeoutProducer(void) {return timeoutProd;}
    void getOption(APP_TYPE::APP_GET_OPTION optType, void *optionValue, appInt optionValueLen, void *returnValue = NULL, appInt returnValueLen = 0);
	virtual UTIL::WorkerThread *getWorker(WorkerType type);

private:
	PacketIpHeader *getIpHeaders(appInt8 *ohcnt);
	appByte *listeningIp;
	appInt localPort;
	appBool threadRunning;
//	struct ev_loop *loop;
	pthread_t threadId;
	ev_timer timer;
	PacketEventHandler *connection;
	sem_t acceptNewFlow;
	std::map<appInt16, ClientDetails *> clients, pendingClients;
	void *newDataCallBackData;
	TimeOutProducer timeoutProd;
	appInt8 primaryInterfaceId;
	virtual appSInt readData(appInt16 flowId, appByte *data, appInt size) {APP_ASSERT_MSG(0, "INVALID"); return 0;};
	virtual appSInt sendData(appInt16 flowId, appByte *data, appInt size) {APP_ASSERT_MSG(0, "INVALID"); return 0;};
    void setupIterface();
    validateNewClientCallBack validateNewClient;
    newFlowCallBack newFlowCB;
    std::map<WorkerType, UTIL::WorkerThread *> workers;
    std::mutex getWorkerMutex;
};


#endif /* SRC_TUNNELLIB_SERVERCONNECTION_HPP_ */
