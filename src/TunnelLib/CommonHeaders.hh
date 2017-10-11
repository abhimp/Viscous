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
 * CommonHeaders.h
 *
 *  Created on: 04-Aug-2016
 *      Author: abhijit
 */
#ifndef NEW_CHANNEL_HANDLE
#define NEW_CHANNEL_HANDLE
#endif

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

#include "../util/ThreadPool.hh"
#include "Packet.h"
#include "appTypes.h"
#include "PendingAcks.hh"

//#define ev_loop pop
#define SIZE_MB(_x) (_x*1024*1024)
#define SIZE_8MB SIZE_MB(8)
#define TIMER_GRANULITY_MS 200.0
#define TIMER_GRANULITY_US 200000.0
#define TIMER_SECOND_IN_MS 1000
#define TIMER_SECOND_IN_US 1000000
class TimeOutProducer;
class InterfaceMonitor;

enum error_list{
    ERROR_INVALID_READ_SIZE = 2,
    ERROR_INVALID_PACKET,
    ERROR_DUPLICATE_PACKET,
    ERROR_NO_DATA,
    ERROR_INVALID_FLOW,
    ERROR_FLOW_CLOSED,
    ERROR_NETWORK_CLOSED,
};


#define hasKey(_map, _key) (_map.find(_key) != _map.end())

class appTs{
public:
    appTs(){ts.tv_nsec = 0; ts.tv_sec = 0;}
    appTs(appInt64 sec, appInt64 nsec){ts.tv_sec = sec; ts.tv_nsec = nsec;}
    appTs(appSInt64 us){ts.tv_sec = us/1000000L; ts.tv_nsec = (us % 1000000L)*1000;}
    inline appInt64 &sec(){return ts.tv_sec;}
    inline appInt64 &nsec(){return ts.tv_nsec;}
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
    appTime ts;
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
    static const appInt16 FLAG_CHS = 1<<6; //start Channel syn
    static const appInt16 FLAG_CSN = 1<<7;
    static const appInt16 FLAG_CTR = 1<<8; //control packet
    static const appInt16 FLAG_CHF = 1<<9;
    static const appInt16 FLAG_IFC = 1<<10; //Interface related event;
};

struct RecvSendFlags{
    appInt16 newFlow:1;
};

#define DEFALT_RECVSENDFLAG_VALUE {FALSE}

class AppTimeClass{
public:
    virtual appTs getTime(){appTime x; appGetSysTime(&x); appTs y(0,0); y.nsec() = x.tv_nsec; y.sec() = x.tv_sec; return y;}
    virtual ~AppTimeClass(){}
};

union ClientFlowId{
    ClientFlowId():clientFlowId(0){}
    struct{
        appInt16 fingerPrint;
        appInt16 flowId;
    };
    appInt32 clientFlowId;
};
typedef ClientFlowId appFlowIdType;

class BaseReliableObj : public TimeoutObserver, public PacketFlags, public AppTimeClass{
public:
    BaseReliableObj(BaseReliableObj *parent);
    virtual ~BaseReliableObj();
    virtual appSInt sendPacketTo(appInt id, Packet *pkt) = 0;
    virtual appSInt recvPacketFrom(Packet *pkt, RecvSendFlags &flags) = 0;
    virtual appTs getTime(){if(parent) return parent->getTime(); else{appTime x; appGetSysTime(&x); appTs y(0,0); y.nsec() = x.tv_nsec; y.sec() = x.tv_sec; return y;}}
    virtual appInt timeoutEvent(appTs time) = 0;
    inline virtual appInt getId(void){return id;}
    virtual appSInt readData(appFlowIdType flowId, appByte *data, appInt size){assert(0); return 0;};
    virtual appSInt sendData(appFlowIdType flowId, appByte *data, appInt dataLen){assert(0); return 0;};
    inline virtual appSInt recvAck(appFlowIdType flowId, appInt16 flowSeqNo){assert(parent); return parent->recvAck(flowId, flowSeqNo);};

    virtual appStatus closeFlow(appFlowIdType flowId) = 0;
//    virtual void getOption(APP_TYPE::APP_GET_OPTION optType, void *optionValue, appInt optionValueLen, void *returnValue = NULL, appInt returnValueLen = 0){APP_ASSERT(0 && "NOT IMPLEMENTED");};
//    virtual inline PendingAcks &pendingAcks(void) {APP_ASSERT(parent); return parent->pendingAcks();}
    virtual inline TimeOutProducer &timeoutProducer(void) {APP_ASSERT(parent); return parent->timeoutProducer();}
    virtual inline appInt16 getFingerPrint(void) {APP_ASSERT(parent); return parent->getFingerPrint();}
    virtual inline appBool isServer(void) {APP_ASSERT(parent); return parent->isServer();}
    virtual inline appBool isClient(void) {APP_ASSERT(parent); return parent->isClient();}
    virtual inline PacketReadHeader *getReceiverStatus(){APP_ASSERT(parent); return parent->getReceiverStatus();}
    virtual inline InterfaceMonitor *getInterfaceMontor(){APP_ASSERT(parent); return parent->getInterfaceMontor();}
    virtual inline appSInt recvProcPacket(Packet *pkt){APP_ASSERT(parent); return parent->recvProcPacket(pkt);}
    typedef enum{
        STREAM_COMMON_WORKER
    }WorkerType;
    virtual util::WorkerThread *getWorker(WorkerType type) {if(parent) return parent->getWorker(type); else return NULL;}
    typedef enum{
        SIMPLE_RELIABILITY,
        STOP_N_WAIT,
        STREAM_HANDLER,
        NEW_FLOW_HANDLER,
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
    ReliabilityMod(BaseReliableObj *parent, appFlowIdType flowId) :
            BaseReliableObj(parent), flowId_(flowId){}

    virtual ~ReliabilityMod(){};

    virtual appInt timeoutEvent(appTs time) = 0;
    virtual appStatus recvAck(PacketAckHeader *pktAckHdr) = 0;
    virtual appSInt recvAck(appFlowIdType flowId, appInt16 flowSeqNo) {APP_ASSERT("ERROR" && 0); return 0;};
    virtual appFlowIdType &flowId(){return flowId_;}
//    virtual void initiateClosure() = 0;
    virtual appInt16 getReadUpto() = 0;
    typedef void (*newDataToFlow)(void *info, appByte *data, appInt dataLen);
    typedef void (*closingFlow)(void *info);
    typedef enum{
        EVENT_INVALID,
        EVENT_NEW_DATA,
        EVENT_CLOSING,
    }EventType;
    virtual void setCallBack(EventType evt, void *dataToPass, void *func) = 0;
protected:
    appFlowIdType flowId_;
};

class TimeOutProducer{
public:
    TimeOutProducer():listner(){};
    void timeoutEvent(appTs time);
    void attach(TimeoutObserver *);
    void detach(TimeoutObserver *);
private:
    std::set<TimeoutObserver *> listner;
    util::AppMutex accessMutex;
};
#endif /* TUNNELLIB_COMMONHEADERS_HPP_ */
