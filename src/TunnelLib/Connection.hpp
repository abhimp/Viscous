/*
 * Conection.hpp
 *
 *  Created on: 07-Aug-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_CONNECTION_HPP_
#define SRC_TUNNELLIB_CONNECTION_HPP_

#include "CommonHeaders.hpp"
#include <common.h>
#include <cstring>
#include "../../libev/ev++.h"

class Connection:public BaseReliableObj{
public:
	Connection(BaseReliableObj *parent): BaseReliableObj(parent), localAddrLen(sizeof(localAddr)), socketFd(0), allowNewCon(FALSE){std::memset(&localAddr, 0, sizeof(localAddr));};
	virtual ~Connection(){};
	virtual appSInt sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen);
	virtual appSInt recvPacketFrom(Packet *pkt) {APP_ASSERT_MSG(1, "YOU SHOULD NOT CALL IT. CHECK YOUR CODE\n"); return 0; };
	virtual appSInt sendData(appByte *data, appInt dataLen){return -1;};
//	virtual appTs getTime(){if(parent) return parent->getTime(); else return ev_time();}
	virtual appInt timeoutEvent(appTs time) {APP_ASSERT_MSG(1, "YOU SHOULD NOT CALL IT. CHECK YOUR CODE\n"); return 0; };
	virtual appStatus startClient(appInt localPort, appByte *localIp = (appByte *)"0.0.0.0");
	virtual appStatus startServer(appInt localPort, appByte *localIp = (appByte *)"0.0.0.0");
	virtual void dataRecvCb(EV_P_ ev_io *w, int revents);
	virtual appStatus reset(appInt16 flowid){APP_ASSERT("INVALID CALL" && 0); return APP_SUCCESS;};
	virtual appStatus closeFlow(appInt16 flowid){APP_ASSERT("INVALID CALL" && 0); return APP_SUCCESS;};
	virtual appInt getLocalPort(void);
	virtual void close(void);
	virtual void listen();
private:
	sockaddr_in localAddr;
	socklen_t localAddrLen;
	appSInt socketFd;
	ev_io watcher;
	appBool allowNewCon;
};

#endif /* SRC_TUNNELLIB_CONNECTION_HPP_ */
