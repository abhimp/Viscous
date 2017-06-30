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
 * CommonHeaders.h
 *
 *  Created on: 04-Aug-2016
 *      Author: abhijit
 */

#ifndef TUNNELLIB_COMMONHEADERS_HPP_
#define TUNNELLIB_COMMONHEADERS_HPP_
#include <common.h>
#include <network.h>
#include <iostream>
#include <net/ethernet.h>

//#include <ev++.h>
//#include "../../libev/ev++.h"
#include <map>
#include <set>
#include <queue>
#include <vector>
#include <semaphore.h>
#include "Packet.h"
#include "PendingAcks.h"
#include "appTypes.h"
#include "../util/ThreadPool.h"

//#define ev_loop pop
#define SIZE_MB(_x) (_x*1024*1024)
#define SIZE_8MB SIZE_MB(8)
#define TIMER_GRANULITY_MS 200.0
#define TIMER_GRANULITY_US 200000.0
#define TIMER_SECOND_IN_MS 1000
#define TIMER_SECOND_IN_US 1000000
class TimeOutProducer;



#define hasKey(_map, _key) (_map.find(_key) != _map.end())

class appTs{
public:
	appTs(){ts.tv_nsec = 0; ts.tv_sec = 0;}
	appTs(time_t sec, long nsec){ts.tv_sec = sec; ts.tv_nsec = nsec;}
	appTs(appSInt64 us){ts.tv_sec = us/1000000L; ts.tv_nsec = (us % 1000000L)*1000;}
	inline time_t &sec(){return ts.tv_sec;}
	inline long &nsec(){return ts.tv_nsec;}
	bool operator> (appTs nts);
	bool operator<(appTs nts);
	appTs operator+(double n);
	appTs operator+(appTs oth);
	appTs addmili(appInt64 ms);
	appTs addmicro(appInt64 us);
	appTs addnano(appInt64 ns);
	appSInt64 getMicro();
	appSInt64 getMili();

	void reInit(){sec() = 0; nsec() = 0;}
private:
	struct timespec ts;
};
std::ostream& operator<< (std::ostream& os, appTs ts);

class TimeoutObserver{
public:
	virtual appInt timeoutEvent(appTs time) = 0;
	virtual ~TimeoutObserver(){};
};

class PacketFlags{
public:
	static const appInt16 FLAG_ACK = 1<<0;
	static const appInt16 FLAG_DAT = 1<<1; //data
	static const appInt16 FLAG_SYN = 1<<2; //Setup connection
	static const appInt16 FLAG_FFN = 1<<3; //Flow finished. need to send a ack;
	static const appInt16 FLAG_CFN = 1<<4; //Client finished
	static const appInt16 FLAG_FAC = 1<<5; //flow ack
	static const appInt16 FLAG_PSH = 1<<6; //start sending data
	static const appInt16 FLAG_CSN = 1<<7;
};

class AppTimeCla{
public:
	virtual appTs getTime(){appTime x; appGetSysTime(&x); appTs y(0,0); y.nsec() = x.tv_nsec; y.sec() = x.tv_sec; return y;}
	virtual ~AppTimeCla(){}
};

class BaseReliableObj : public TimeoutObserver, public PacketFlags, public AppTimeCla{
public:
	BaseReliableObj(BaseReliableObj *parent);
	virtual ~BaseReliableObj();
	virtual appSInt sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen) = 0;
	virtual appSInt recvPacketFrom(Packet *pkt) = 0;
	virtual appTs getTime(){if(parent) return parent->getTime(); else{appTime x; appGetSysTime(&x); appTs y(0,0); y.nsec() = x.tv_nsec; y.sec() = x.tv_sec; return y;}}
	virtual appInt timeoutEvent(appTs time) = 0;
	inline virtual appInt getId(void){return id;}
	virtual appSInt readData(appInt16 flowId, appByte *data, appInt size) {assert(0); return 0;};
	virtual appSInt sendData(appInt16 flowId, appByte *data, appInt dataLen) {assert(0); return 0;};
	inline virtual appSInt recvAck(appInt16 flowId, appInt16 flowFeqNo){assert(parent); return parent->recvAck(flowId, flowFeqNo);};

	virtual appStatus closeFlow(appInt16 flowId) = 0;
    virtual void getOption(APP_TYPE::APP_GET_OPTION optType, void *optionValue, appInt optionValueLen, void *returnValue = NULL, appInt returnValueLen = 0){APP_ASSERT(0 && "NOT IMPLEMENTED");};
	virtual inline PendingAcks &pendingAcks(void) {APP_ASSERT(parent); return parent->pendingAcks();}
	virtual inline TimeOutProducer &timeoutProducer(void) {APP_ASSERT(parent); return parent->timeoutProducer();}
	virtual inline appInt16 getFingerPrint(void) {APP_ASSERT(parent); return parent->getFingerPrint();}
	typedef enum{
	    STREAM_COMMON_WORKER
	}WorkerType;
	virtual UTIL::WorkerThread *getWorker(WorkerType type) {if(parent) return parent->getWorker(type); else return NULL;}
	typedef enum{
		SIMPLE_RELIABILITY,
		STOP_N_WAIT,
		STREAM_HANDLER,
	}CongType;

protected:
	BaseReliableObj *parent;
	appInt id;

	appInt getNextChildId();

private:
	static appInt nextChildId;
	static std::set<appInt> childIdPool, childIdFreePool;
	static pthread_mutex_t newIdLock;
};


class ReliabilityMod: public BaseReliableObj {
public:
	ReliabilityMod(BaseReliableObj *parent, appInt16 flowId) :
			BaseReliableObj(parent), flowId_(flowId){}

	virtual ~ReliabilityMod(){};

	virtual appInt timeoutEvent(appTs time) = 0;
	virtual appStatus recvAck(PacketAckHeader *pktAckHdr) = 0;
	virtual appInt16 &flowId(){return flowId_;}
	virtual void initiateClosure() = 0;
	typedef void (*newDataToFlow)(void *info, appByte *data, appInt dataLen);
	typedef void (*closingFlow)(void *info);
	typedef enum{
		EVENT_INVALID,
		EVENT_NEW_DATA,
		EVENT_CLOSING,
	}EventType;
    virtual void setCallBack(EventType evt, void *dataToPass, void *func) = 0;
protected:
	appInt16 flowId_;
};

class TimeOutProducer{
public:
	TimeOutProducer():listner(){};
	void timeoutEvent(appTs time);
	void attach(TimeoutObserver *);
	void detach(TimeoutObserver *);
private:
	std::set<TimeoutObserver *> listner;
	std::mutex accessMutex;
};
#endif /* TUNNELLIB_COMMONHEADERS_HPP_ */
