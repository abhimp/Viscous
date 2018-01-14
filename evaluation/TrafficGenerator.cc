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
 * TrafficGenerator.cc

 *
 *  Created on: 27-Aug-2016
 *      Author: abhijit
 */

#include "../src/TunnelLib/ServerConnection.hh"
#include "../src/TunnelLib/ClientConnection.hh"
#include <appThread.h>
#include <sstream>
#include <fstream>
#include "../src/TunnelLib/FlowHandler/Streamer.hh"
#include "../src/TunnelLib/InterfaceController/arpResolv.h"
#include "TrafficGenerator.h"

#include "../src/util/Logger.hh"
#include "../src/util/ThreadPool.hh"
#include "test_distribution.h"
#include "../src/util/AppThread.hh"

namespace TrafficGeneratorServer{

struct SconData{
public:
    server::ServerConnection sCon;
    std::string filePathPrefix;
    appBool saveToFile;
    util::AppSemaphore waitToClose;
    SconData(appInt localPort):sCon(localPort), filePathPrefix("/tmp/MultiPath-"), saveToFile(FALSE){};
    SconData(appInt localPort, appByte *fpath):sCon(localPort), filePathPrefix((char *)fpath), saveToFile(FALSE){};
};

class Server : public util::AppThread{
private:
    SconData *scond;
    appInt32 flowId;
public:
    Server(SconData *scond, appInt32 flowId):scond(scond), flowId(flowId){}
    void run();
};

void Server::run(){
    appByte data[2048];
    appInt size = 2048;
    appSInt len = 0;
    std::ofstream fyle;
    appChar dt[2048];
    if(scond->saveToFile and !scond->filePathPrefix.empty()){
//        std::stringstream stream;
        sprintf((char *)dt, "%s%d", scond->filePathPrefix.c_str(), flowId);
//        stream << scond->filePathPrefix << flowId << std::endl;
//        stream >> dt;
        fyle.open((char *)dt);
    }
    while(1){
        len = scond->sCon.readData(flowId, data, size);
        LOGI("Hello recved %d", len);
        if(len == 0)
            break;
        if(scond->saveToFile and fyle.is_open())
            fyle.write((char *)data, len);
    }
    scond->sCon.closeFlow(flowId);
    if(scond->saveToFile and fyle.is_open()){
        fyle.flush();
        fyle.close();
    }
//    delete this;
    scond->waitToClose.notify();
//    sleep(10);
//    exit(1); //TODO
}

#define NUM_FLOW_ACCEPT 1

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
    scond.sCon.startServer();
    std::set<Server *> flowIdMap;
    while(1){
        auto flowId = scond.sCon.acceptFlow();
        auto x = new Server(&scond, flowId);
        x->start();
    }
    for(auto p = 0; p < NUM_FLOW_ACCEPT; p++){
        auto flowId = scond.sCon.acceptFlow();
//        assert(!(hasKey(flowIdMap, flowId)));
        auto x = new Server(&scond, flowId);
        x->start();
        flowIdMap.insert(x);
//        flowIdMap.insert(flowId);
    }

    for(auto p = 0; p < NUM_FLOW_ACCEPT; p++){
        scond.waitToClose.wait();
    }
    sleep(6);
    for(auto x : flowIdMap){
        delete x;
    }
    sleep(6);
}
}

namespace TrafficGeneratorClient{
class Client : public util::AppThread{
private:
    ClientConnection *cCon;
    appInt32 flowId;
    appByte *fpath;
public:
    Client(ClientConnection *cCon, appInt32 flowId, appByte *fpath): cCon(cCon), flowId(flowId), fpath(fpath){}
    void run();
};

void Client::run(){
    FILE *fp = fopen((const char *)fpath, "r");
    appByte dt[1290];
    int len;
    while(feof(fp) == 0){
        len = fread(dt, 1, 1290, fp);
        if(len == 0)
            break;
        cCon->sendData(flowId, dt, len);
    }
    fclose(fp);
    cCon->closeFlow(flowId);
}

void startClient(appByte *serverIp, appInt serverPort, appByte *fpath){
    ClientConnection cCon(serverIp, serverPort);
    if(cCon.startClient() == APP_SUCCESS){
        auto flowId = cCon.addNewFlow();
        auto cl = new Client(&cCon, flowId, fpath);
        cl->run();
    }
    cCon.close();
}
}

namespace TrafficGeneratorClient2{

class Client : public util::AppThread{
private:
    ClientConnection *cCon;
//    appInt32 flowId;
    appInt flowCount;
//    appByte *fpath;
    util::AppSemaphore *threadEndNotifier;
public:
    Client(ClientConnection *cCon, appInt flowCount, util::AppSemaphore *sem): cCon(cCon), flowCount(flowCount), threadEndNotifier(sem){}
    void run();
};

void Client::run(){
    appInt32 flowId;
    appByte dt[1290];
    appInt dtLen = sizeof(dt);

    for(appInt x = 0; x < sizeof(dt); x++){
        dt[x] = std::rand();
    }

    for(appInt index = 0; index < flowCount; index++){
        appInt count = staticDistribution::TEST_EXPOENETIAL_DISTRIBUTION[index % (staticDistribution::TEST_EXPOENETIAL_DISTRIBUTION_LEN)];
        flowId = cCon->addNewFlow();
        LOGI("testflowtrace2: %d, %ld", flowId, cCon->getTime().getMicro());
        for(; count; count --){
            LOGI("testflowtrace1: %d, %ld", flowId, cCon->getTime().getMicro())
            cCon->sendData(flowId, dt, dtLen);
        }
        LOGI("testflowtrace1: %d, %ld", flowId, cCon->getTime().getMicro())
        cCon->closeFlow(flowId);
        LOGI("testflowtrace2: %d, %ld", flowId, cCon->getTime().getMicro());
    }
    threadEndNotifier->notify();
}

void startClient(appByte *serverIp, appInt serverPort, appInt numThread, appInt numFlow){
    util::AppSemaphore waitToFinish;
    ClientConnection cCon(serverIp, serverPort);
    appInt closeCout = 0;
    if(cCon.startClient() == APP_SUCCESS){
        for(appInt x = 0; x < numThread; x++){
            auto cl = new Client(&cCon, numFlow, &waitToFinish);
            cl->start();
        }
    }

    while(closeCout != numThread){
        waitToFinish.wait();
        closeCout ++;
    }

    cCon.close();
}
}

namespace TrafficGeneratorClient3{

void startClient(appByte *serverIp, appInt serverPort, appInt numThread, appInt numPackets){
    LOGE("NOT IMPLEMENTED");
}
}
