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
 * TrafficGenerator.cc

 *
 *  Created on: 27-Aug-2016
 *      Author: abhijit
 */

#include "../src/TunnelLib/ServerConnection.hpp"
#include "../src/TunnelLib/ClientConnection.hpp"
#include <appThread.h>
#include <sstream>
#include <fstream>
#include "../src/TunnelLib/ARQ/Streamer.h"
#include "../src/TunnelLib/InterfaceController/arpResolv.h"
#include "../../util/Logger.h"
#include "TrafficGenerator.h"
#include "../src/util/ThreadPool.h"
#include "test_distribution.h"

namespace TrafficGeneratorServer{

struct SconData{
public:
    ServerConnection sCon;
    std::string filePathPrefix;
    appBool saveToFile;
    SconData(appInt localPort):sCon(localPort), filePathPrefix("/tmp/MultiPath-"), saveToFile(FALSE){};
    SconData(appInt localPort, appByte *fpath):sCon(localPort), filePathPrefix((char *)fpath), saveToFile(FALSE){};
};

struct ClientInformation{
    appInt16 fingerPrint;
};

struct FlowInfo{
    FlowInfo():sCon(NULL), scond(NULL), fingerPrint(0), newFlow(0), streamer(NULL), ofyle(NULL), closed(FALSE), flowStartedAt(0), bytesReceived(0){
        AppTimeCla time;
        flowStartedAt = time.getTime().getMicro();
    }
    ServerConnection *sCon;
    SconData *scond;
    appInt16 fingerPrint;
    appInt16 newFlow;
    ARQ::Streamer *streamer;
    std::ofstream *ofyle;
    appBool closed;
    appInt64 flowStartedAt;
    appInt64 bytesReceived;
};

appBool validateNewClientCallBack(ServerConnection *sCon, Packet *pkt, sockaddr_in &src_addr){
    return TRUE;
}

void *recvData(void *data, appThreadInfoId tid){
    ServerConnection *sCon;
    SconData *scond;
    appInt16 fingerPrint;
    appInt16 newFlow;
    ARQ::Streamer *streamer;

    APP_UNPACK((appByte *)data, sCon, fingerPrint, newFlow, streamer);
    scond = (SconData *)sCon;
    appSInt len;
    appByte dt[2048];
    std::ofstream *ofyle = NULL;
    if(scond->saveToFile and !scond->filePathPrefix.empty()){
        std::stringstream stream;
        stream << scond->filePathPrefix << fingerPrint << "-" << newFlow << std::endl;
        stream >> dt;
        ofyle = new std::ofstream((char *)dt);
    }
    LOGGER::Logger *lgr = new LOGGER::Logger((char *)"/tmp/test.log");
    while((len=streamer->readData(dt, sizeof(dt))) > 0){
        LOG_WRITE_D((*lgr), "time: %ld, read: %d", sCon->getTime().getMicro(), len);
        if(len && ofyle)
            ofyle->write((char *)dt, len);
    }
    if(ofyle){
        ofyle->flush();
        ofyle->close();
    }
    return NULL;
}


void newDataToFlow(void *info, appByte *data, appInt dataLen){
    FlowInfo *finfo = (FlowInfo *)info;
//    LOG_WRITE_D((*lgr), "time: %ld, read: %d", finfo->sCon->getTime().getMicro(), len);
    APP_ASSERT(!finfo->closed);
    if(dataLen && finfo->ofyle)
        finfo->ofyle->write((char *)data, dataLen);
    finfo->bytesReceived += dataLen;
    auto timeNow = finfo->sCon->getTime().getMicro();
    auto timeElapsed = timeNow - finfo->flowStartedAt;
    LOGI("DATA received at mpiot receiver: %d, %ld, %ld, %ld", finfo->newFlow, finfo->bytesReceived, timeElapsed, timeNow);
}
void closingFlow(void *info){
    FlowInfo *finfo = (FlowInfo *)info;
    APP_ASSERT(!finfo->closed);
    if(finfo->ofyle){
        finfo->ofyle->close();
        finfo->ofyle = NULL;
    }
    finfo->closed = TRUE;
}

void newFlowCallBack(ServerConnection *sCon, appInt16 fingerPrint, appInt16 newFlow, ARQ::Streamer *streamer){
//    appByte *buf;
//    buf = APP_PACK(sCon, fingerPrint, newFlow, streamer);
    std::ofstream *ofyle = NULL;
    SconData *scond = (SconData *)sCon;
    appByte dt[2048];
   if(scond->saveToFile and !scond->filePathPrefix.empty()){
       std::stringstream stream;
       stream << scond->filePathPrefix << fingerPrint << "-" << newFlow << std::endl;
       stream >> dt;
       ofyle = new std::ofstream((char *)dt);
   }
   FlowInfo *finfo = new FlowInfo();
   finfo->fingerPrint = fingerPrint;
   finfo->newFlow = newFlow;
   finfo->ofyle = ofyle;
   finfo->sCon = sCon;
   finfo->scond = scond;
   finfo->streamer = streamer;
   streamer->setCallBack(ReliabilityMod::EVENT_NEW_DATA, finfo, (void *)newDataToFlow);
   streamer->setCallBack(ReliabilityMod::EVENT_CLOSING, finfo, (void *)closingFlow);
//    runInThreadGetTid(recvData, buf, FALSE, NULL);
}

void startServer(appInt localPort, appByte *fylePathPrefix){
    LOGI("startServer");
    SconData scond(localPort);
    LOGI("startServer");
    if(fylePathPrefix){
        LOGI("startServer");
        scond.filePathPrefix = (char *)fylePathPrefix;
        LOGI("startServer");
        scond.saveToFile = TRUE;
        LOGI("startServer");
    }
	scond.sCon.setClientValidator(validateNewClientCallBack);
    LOGI("startServer");
	scond.sCon.setNewFlowCallBack(newFlowCallBack);
    LOGI("startServer");
	scond.sCon.startServer();
    LOGI("startServer");
	scond.sCon.waitToJoin();
    LOGI("startServer");
}
}

namespace TrafficGeneratorClient2{
struct FlowInfo{
    FlowInfo(): cCon(NULL), index(0), flowId(0), streamer(NULL), ofyle(NULL), closed(FALSE), threadIndex(0), sem(NULL){}
    ClientConnection *cCon;
    appSInt index;
    appInt16 flowId;
    ARQ::Streamer *streamer;
    std::ofstream *ofyle;
    appBool closed;
    appInt threadIndex;
    AppSemaphore *sem;
};

void newDataToFlow(void *info, appByte *data, appInt dataLen){

}
void flowClosing(void *info){
    FlowInfo *finfo = (FlowInfo *)info;
    finfo->sem->notify();
    LOGI("testflowtrace2: %d, %ld", finfo->flowId, finfo->cCon->getTime().getMicro());
}

void *startSendingDataInsideThread(void *data){
	ClientConnection *cCon = NULL;
	ARQ::Streamer *flow;
	appSInt numFlow;
	appByte *dt;
	appInt dtLen;
	AppSemaphore *semParent, sem;
	appInt threadIndex;
	APP_UNPACK((appByte *)data, cCon, threadIndex, numFlow, dt, dtLen, semParent);
	for(appInt index = 0; index < numFlow; index++){
//	    LOGE("starting flow: %d %d", threadIndex, index)
        FlowInfo *finfo = new FlowInfo();
        finfo->cCon = cCon;
        finfo->index = index;
        finfo->sem = &sem;
        finfo->threadIndex = threadIndex;
        appInt count = staticDistribution::TEST_EXPOENETIAL_DISTRIBUTION[index % (staticDistribution::TEST_EXPOENETIAL_DISTRIBUTION_LEN)];//staticDistribution::exponential2[index%(staticDistribution::exponential2Len)];
        flow = cCon->addNewFlow();
//        LOGI("testflowtrace2: %d, %ld", flow->flowId(), cCon->getTime().getMicro())
        finfo->streamer = flow;
        finfo->flowId = flow->flowId();
        flow->setCallBack(ReliabilityMod::EVENT_NEW_DATA, finfo, (void *)newDataToFlow);
        flow->setCallBack(ReliabilityMod::EVENT_CLOSING, finfo, (void *)flowClosing);
        for(; count; count --){
            LOGI("testflowtrace1: %d, %ld", finfo->flowId, cCon->getTime().getMicro())
            flow->sendData(dt, dtLen);
        }

        LOGI("testflowtrace1: %d, %ld", finfo->flowId, cCon->getTime().getMicro())
    //    cCon->closeFlow(flow->flowId());
        flow->initiateClosure();
        LOGI("flow closed");
	}
	for(appInt index = 0; index < numFlow; index++)
	    sem.wait();
	semParent->notify();
	return NULL;
}

void startSendingData(ClientConnection &cCon, appInt numThread, appInt numFlows){
    UTIL::ThreadPool pool(numThread);
    appByte *buf;
    pool.run();

    appByte dt[1290];
    appInt dtLen = sizeof(dt);
    auto ptr = &cCon;

    AppSemaphore semObj(numThread);
    auto sem = &semObj;

    for(appInt x = 0; x < sizeof(dt); x++){
        dt[x] = std::rand();
    }

    buf = dt;
    appInt clientCnt = 0;
    for(appInt i = 0; i < numThread; i++){
        auto data = APP_PACK(ptr, i, numFlows, buf, dtLen, sem);
        pool.executeInsidePool(startSendingDataInsideThread, data);
        clientCnt ++;
//        sem->wait();
    }
    while(clientCnt){
        sem->wait();
        clientCnt--;
    }
    pool.stop();
//    cCon.close();
}

void startClient(appByte *serverIp, appInt serverPort, appInt numThread, appInt numPackets){
	ClientConnection cCon(serverIp, serverPort);
	if(cCon.startClient() == APP_SUCCESS){
	    startSendingData(cCon, numThread, numPackets);
	    cCon.close();
	    cCon.waitToJoin();
	}
}
}

namespace TrafficGeneratorClient{
void newDataToFlow(void *info, appByte *data, appInt dataLen){

}

void *startSendingData(void *data, appThreadInfoId tid){
	ClientConnection *cCon = NULL;
	ARQ::Streamer *flow;
	APP_UNPACK((appByte *)data, cCon, flow);
	appByte dt[1290];
	for(appInt x = 0; x < sizeof(dt); x++){
	    dt[x] = std::rand();
	}
	appInt count = 100;
	for(count = 5; count; count --){
	    flow->sendData(dt, sizeof(dt));
	}
//	flow->close();
	cCon->closeFlow(flow->flowId());
	LOGI("flow closed");
	exit(0);
	return NULL;
}


void startSendingData(ClientConnection &cCon){
    appByte *buffer;
    ClientConnection *ccon = &cCon;
    ARQ::Streamer *newFlow = cCon.addNewFlow();
    buffer = APP_PACK(ccon, newFlow);
    std::stringstream stream;
    runInThreadGetTid(startSendingData, buffer, FALSE, NULL);
}

void startSendingData(ClientConnection &cCon, appByte *fpath){
    FILE *fp = fopen((const char *)fpath, "r");
    appByte dt[1290];
    int len;
    auto newFlow = cCon.addNewFlow();
    newFlow->setCallBack(ReliabilityMod::EVENT_NEW_DATA, &cCon, (void *)newDataToFlow);
    while(feof(fp) == 0){
        len = fread(dt, 1, 1290, fp);
        newFlow->sendData(dt, len);
    }
    fclose(fp);
}

void startClient(appByte *serverIp, appInt serverPort, appByte *fpath){
	ClientConnection cCon(serverIp, serverPort);
	if(cCon.startClient() == APP_SUCCESS){
	    startSendingData(cCon, fpath);
	    cCon.close();
	    cCon.waitToJoin();
	}
}
}

namespace TrafficGeneratorClient3{
struct FlowInfo{
    FlowInfo(): cCon(NULL), index(0), flowId(0), streamer(NULL), ofyle(NULL), closed(FALSE), sem(NULL){}
    ClientConnection *cCon;
    appSInt index;
    appInt16 flowId;
    ARQ::Streamer *streamer;
    std::ofstream *ofyle;
    appBool closed;
    AppSemaphore *sem;
};

void newDataToFlow(void *info, appByte *data, appInt dataLen){

}
void flowClosing(void *info){
    FlowInfo *finfo = (FlowInfo *)info;
    finfo->sem->notify();
    LOGI("testflowtrace2: %d, %ld", finfo->index, finfo->cCon->getTime().getMicro());
}

void *startSendingDataInsideThread(void *data){
	ClientConnection *cCon = NULL;
	ARQ::Streamer *flow;
	appSInt index;
	appByte *dt;
	appInt dtLen;
	AppSemaphore *sem;
	APP_UNPACK((appByte *)data, cCon, index, dt, dtLen, sem);
	FlowInfo *finfo = new FlowInfo();
	finfo->cCon = cCon;
	finfo->index = index;
	finfo->sem = sem;

	LOGI("testflowtrace2: %d, %ld", index, cCon->getTime().getMicro())

	appInt count = staticDistribution::TEST_EXPOENETIAL_DISTRIBUTION[index % (staticDistribution::TEST_EXPOENETIAL_DISTRIBUTION_LEN)];//staticDistribution::exponential2[index%(staticDistribution::exponential2Len)];
	flow = cCon->addNewFlow();
	finfo->streamer = flow;
	finfo->flowId = flow->flowId();
	flow->setCallBack(ReliabilityMod::EVENT_NEW_DATA, finfo, (void *)newDataToFlow);
	flow->setCallBack(ReliabilityMod::EVENT_CLOSING, finfo, (void *)flowClosing);
	for(; count; count --){
        LOGI("testflowtrace1: %d, %ld", index, cCon->getTime().getMicro())
	    flow->sendData(dt, dtLen);
	}

    LOGI("testflowtrace1: %d, %ld", index, cCon->getTime().getMicro())
//    cCon->closeFlow(flow->flowId());
    flow->initiateClosure();
	LOGI("flow closed");
	return NULL;
}

void startSendingData(ClientConnection &cCon, appInt numThread, appInt numPackets){
    UTIL::ThreadPool pool(numThread);
    appByte *buf;
    pool.run();

    appByte dt[1290];
    appInt dtLen = sizeof(dt);
    auto ptr = &cCon;

    AppSemaphore semObj(numThread);
    auto sem = &semObj;

    for(appInt x = 0; x < sizeof(dt); x++){
        dt[x] = std::rand();
    }

    buf = dt;
    appInt clientCnt = 0;
    for(appInt i = 0; i < numPackets; i++){
        auto data = APP_PACK(ptr, i, buf, dtLen, sem);
        pool.executeInsidePool(startSendingDataInsideThread, data);
        clientCnt ++;
//        sem->wait();
    }
    while(clientCnt){
        sem->wait();
        clientCnt--;
    }
    pool.stop();
//    cCon.close();
}

void startClient(appByte *serverIp, appInt serverPort, appInt numThread, appInt numPackets){
	ClientConnection cCon(serverIp, serverPort);
	if(cCon.startClient() == APP_SUCCESS){
	    startSendingData(cCon, numThread, numPackets);
	    cCon.close();
	    cCon.waitToJoin();
	}
}
}
#if 0
void printUsages(char *progName){
	LOGE("use: %s [c|s]\n", progName);
}
#if 01
int main(int argc, char *argv[]){
    logger = new LOGGER::Logger((char *)"/tmp/test.log");
	if(argc < 2){
		printUsages(argv[0]);
		return 2;
	}
	appInt port;
	appByte *ip;
	appByte *fpath;
	switch(argv[1][0]){
		case 'c':
		case 'C':
			if(argc < 5){
				return 3;
			}
			ip = (appByte *)argv[2];
			port = atoi(argv[3]);
			fpath = (appByte *)argv[4];
			TrafficGeneratorClient::startClient(ip, port, fpath);
			break;
		case 's':
		case 'S':
			if(argc < 3){
				return 4;
			}
			port = atoi(argv[2]);
			TrafficGeneratorServer::startServer(port);
			break;
		default:
			return 23;
	}
	delete logger;
	return 0;
}
#else
int main(int argc, char *argv[]){
    auto ips = SearchMac::getInterfaceAndIp();
    appInt32 ip = inet_addr(argv[1]);
    for(auto it:ips){
        arpmain(it.ip.s_addr, ip, it.ifindex);
    }
}
#endif
#endif
