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
 * PacketEventHandler.h
 *
 *  Created on: 23-Mar-2017
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_PACKETEVENTHANDLER_H_
#define SRC_TUNNELLIB_PACKETEVENTHANDLER_H_

#include "CommonHeaders.hpp"
#include "../../util/ConditonalWait.hpp"
#include "Connection.hpp"
#include <mutex>
#include <appThread.h>


class PacketEventHandler : public BaseReliableObj {
public:
    PacketEventHandler(BaseReliableObj *parent);
    virtual ~PacketEventHandler();

	virtual appSInt sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen){return con->sendPacketTo(id, pkt, dest_addr, addrlen);}
	virtual appSInt recvPacketFrom(Packet *pkt);
	virtual appSInt sendData(appByte *data, appInt dataLen){return -1;};
//	virtual appTs getTime(){if(parent) return parent->getTime(); else return ev_time();}
	virtual appInt timeoutEvent(appTs time);
	virtual appStatus startClient(appInt localPort, appByte *localIp = (appByte *)"0.0.0.0");
	virtual appStatus startServer(appInt localPort, appByte *localIp = (appByte *)"0.0.0.0");
	virtual void start();
//	virtual void dataRecvCb(EV_P_ ev_io *w, int revents);
	virtual appStatus reset(appInt16 flowid){APP_ASSERT("INVALID CALL" && 0); return APP_SUCCESS;};
	virtual appStatus closeFlow(appInt16 flowid){APP_ASSERT("INVALID CALL" && 0); return APP_SUCCESS;};
	inline virtual appInt getLocalPort(void){return con->getLocalPort();};
	virtual void close(void);
	static void evTimerExpired(EV_P_  ev_timer *w, int revents);
	void evTimerExpired();
	static void *startBigLoopInsideThread(void *data, appThreadInfoId tid);
	static void *startEventListnerInsideThread(void *data, appThreadInfoId tid);
    static void *startConListnerInsideThread(void *data, appThreadInfoId tid);
    static void *startTimerInsideThread(void *data, appThreadInfoId tid);
	void waitToFinishAllThreads();
	void bigLoop();

private:
	Packet *getNextPacket();
	void addNextPacket(Packet *pkt);
//    BaseReliableObj *conHandler;
	appThreadInfoId bigLoopTid, evLoopTid, conLoopTid, timerLoopTid;
	appBool bigLoopThreadRunning, evLoopThreadRunning, conLoopThreadRunning, timerLoopThreadRunning;
	appBool stopEvThread;
	Connection *con;
    Packet *begQ, *endQ;
    appSInt queueSize;
    appBool pendingTimeOut, closed;
    ev_timer timer;
    AppSemaphore notify;
    AppSemaphore waitToClose;
    std::mutex queueLock;
    std::mutex readWriteLock;
};

#endif /* SRC_TUNNELLIB_PACKETEVENTHANDLER_H_ */
