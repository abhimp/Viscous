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
 * TcpTrafficGenerator.cc
 *
 *  Created on: 17-Mar-2017
 *      Author: abhijit
 */
#include <common.h>
#include <network.h>
#include <appThread.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include "TcpTrafficGenerator.h"
#include "test_distribution.h"
#include "../src/util/ThreadPool.h"
#include <netinet/tcp.h>
#include "../src/TunnelLib/CommonHeaders.hpp"

namespace TcpTrafficGeneratorReciever{

void *recvConn(void *data, appThreadInfoId tid){
    int clientFd;
    sockaddr_in clientAddr;
    socklen_t clen;
    appInt localPort;
    appByte *fylePathPrefix;
    appInt clientCnt;
    appInt readSize;
    appChar buf[2048];

    AppTimeCla time;
    appInt64 byteReceived = 0;
    auto startTime = time.getTime().getMicro();

    APP_UNPACK((appByte *)data, clientFd, clientAddr, clen, localPort, fylePathPrefix, clientCnt);
    std::cerr << "new client: " << clientCnt << std::endl;
    std::ofstream *ofyle = NULL;
    if(fylePathPrefix){
        std::stringstream stream;
        stream << fylePathPrefix << "-" << localPort << "-" << clientCnt << ".txt" << std::endl;
        stream >> buf;
        ofyle = new std::ofstream((char *)buf);
    }

    while((readSize = recv(clientFd, buf, 2048, 0)) > 0){
        if(ofyle)
            ofyle->write((char *)buf,  readSize);
        byteReceived += readSize;
        auto timeNow = time.getTime().getMicro();
        auto elaspedTime = timeNow - startTime;
        LOGI("DATA received at tcp receiver: %d, %ld, %ld, %ld", clientCnt, byteReceived, elaspedTime, timeNow);
    }
    if(ofyle)
        ofyle->close();

    close(clientFd);

    return NULL;
}

void startServer(appInt localPort, appByte *fylePathPrefix){
    int socketFd, clientFd;
    sockaddr_in clientAddr;
    appByte *buf;
    socklen_t clen = sizeof(clientAddr);
    socketFd = app_tcp_listener(localPort);
    appInt clientCnt = 0;
    while(1){
        clientFd = accept(socketFd, (sockaddr *)&clientAddr, &clen);
        clientCnt ++;
        buf = APP_PACK(clientFd, clientAddr, clen, localPort, fylePathPrefix, clientCnt);
        runInThread(recvConn, buf, FALSE);
    }
}
}

namespace TcpTrafficGeneratorSender{
void startClient(appByte *serverIp, appInt serverPort, appByte *fpath){
    int clientFd;
    appByte buf[2048];
    appInt readlen;
    tcp_info tcpInfo;
    int tcpInfoLen = sizeof(tcpInfo);
    AppTimeCla time;
    auto startTime = time.getTime().getMicro();
    clientFd = app_tcp_client_connect_str(serverPort, (char *)serverIp);
    std::ifstream fin((char *)fpath);
    if(!fin.is_open()){
        std::cerr << "error" << std::endl;
        exit(2);
    }
    while(1){
        fin.read((char *)buf, 2048);
        readlen = fin.gcount();
        if(readlen <= 0)
            break;
        send(clientFd, buf, readlen, 0);
        if ( getsockopt( clientFd, SOL_TCP, TCP_INFO, (void *)&tcpInfo, (socklen_t *)&tcpInfoLen ) == 0 ) {
            auto curTime = time.getTime().getMicro();
            LOGI("TCPINFO: %ld %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
                curTime-startTime,
                tcpInfo.tcpi_last_data_sent,
                tcpInfo.tcpi_last_data_recv,
                tcpInfo.tcpi_snd_cwnd,
                tcpInfo.tcpi_snd_wscale,
                tcpInfo.tcpi_snd_ssthresh,
                tcpInfo.tcpi_rcv_ssthresh,
                tcpInfo.tcpi_rtt,
                tcpInfo.tcpi_rttvar,
                tcpInfo.tcpi_unacked,
                tcpInfo.tcpi_sacked,
                tcpInfo.tcpi_lost,
                tcpInfo.tcpi_retrans,
                tcpInfo.tcpi_fackets
            );
        }

    }
    close(clientFd);
    fin.close();
}
}

namespace TcpTrafficGeneratorSender2{

void *sendData(void *data){
    appByte *serverIp;
    appInt serverPort;
    appInt index;
    appByte *dt;
    appInt dtLen;
    AppSemaphore *sem;
    AppTimeCla time;
    appInt numConn;
    tcp_info tcpInfo;
    int tcpInfoLen = sizeof(tcpInfo);
//    AppTimeCla time;

    APP_UNPACK((appByte *)data, serverIp, serverPort, index, numConn, dt, dtLen, sem);
    sem->notify();
    for(auto conIndex = 0; conIndex < numConn; conIndex++){
        auto conId = index*numConn + conIndex;
        LOGI("testflowtrace2: %d, %ld", conId, time.getTime().getMicro());
        auto clientFd = app_tcp_client_connect_str(serverPort, (char *)serverIp);
        auto count = staticDistribution::TEST_EXPOENETIAL_DISTRIBUTION[index % (staticDistribution::TEST_EXPOENETIAL_DISTRIBUTION_LEN)];
        for(auto i = 0; i < count; i++){
            LOGI("testflowtrace1: %d, %ld", conId, time.getTime().getMicro());
            send(clientFd, dt, dtLen, 0);
            if ( getsockopt( clientFd, SOL_TCP, TCP_INFO, (void *)&tcpInfo, (socklen_t *)&tcpInfoLen ) == 0 ) {
                auto curTime = time.getTime().getMicro();
                LOGI("TCPINFO: %d, %ld %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
                    conId,
                    curTime,
                    tcpInfo.tcpi_last_data_sent,
                    tcpInfo.tcpi_last_data_recv,
                    tcpInfo.tcpi_snd_cwnd,
                    tcpInfo.tcpi_snd_wscale,
                    tcpInfo.tcpi_snd_ssthresh,
                    tcpInfo.tcpi_rcv_ssthresh,
                    tcpInfo.tcpi_rtt,
                    tcpInfo.tcpi_rttvar,
                    tcpInfo.tcpi_unacked,
                    tcpInfo.tcpi_sacked,
                    tcpInfo.tcpi_lost,
                    tcpInfo.tcpi_retrans,
                    tcpInfo.tcpi_fackets
                );
            }
        }
        close(clientFd);
        LOGI("testflowtrace2: %d, %ld", conId, time.getTime().getMicro());
    }
    return NULL;
}
void startClient(appByte *serverIp, appInt serverPort, appInt numThread, appInt numConn){
    appByte *buffer;
    appByte dt[1290];
    appInt dtLen = sizeof(dt);
    AppSemaphore semObj;
    AppSemaphore *sem = &semObj;
    for(appInt x = 0; x < sizeof(dt); x++){
        dt[x] = std::rand();
    }

    appByte *data;
    buffer = dt;

    UTIL::ThreadPool pool(numThread);
    pool.run();
    for(appInt i = 0; i < numThread; i++){
        data = APP_PACK(serverIp, serverPort, i, numConn, buffer, dtLen, sem);
        pool.executeInsidePool(sendData, data);
        sem->wait();
    }

    pool.stop();
}
}

namespace TcpTrafficGeneratorSender3{
void *sendData(void *data){
    appByte *serverIp;
    appInt serverPort;
    appInt index;
    appByte *dt;
    appInt dtLen;
    AppSemaphore *sem;
    AppTimeCla time;
    APP_UNPACK((appByte *)data, serverIp, serverPort, index, dt, dtLen, sem);
    sem->notify();
    LOGI("testflowtrace2: %d, %ld", index, time.getTime().getMicro());
    auto clientFd = app_tcp_client_connect_str(serverPort, (char *)serverIp);
    auto count = staticDistribution::TEST_EXPOENETIAL_DISTRIBUTION[index % (staticDistribution::TEST_EXPOENETIAL_DISTRIBUTION_LEN)];
    for(auto i = 0; i < count; i++){
        LOGI("testflowtrace1: %d, %ld", index, time.getTime().getMicro());
        send(clientFd, dt, dtLen, 0);
    }
    close(clientFd);
    LOGI("testflowtrace2: %d, %ld", index, time.getTime().getMicro());
    return NULL;
}
void startClient(appByte *serverIp, appInt serverPort, appInt numThread, appInt numConn){
    appByte *buffer;
    appByte dt[1290];
    appInt dtLen = sizeof(dt);
    AppSemaphore semObj;
    AppSemaphore *sem = &semObj;
    for(appInt x = 0; x < sizeof(dt); x++){
        dt[x] = std::rand();
    }

    appByte *data;
    buffer = dt;

    UTIL::ThreadPool pool(numThread);
    pool.run();
    for(appInt i = 0; i < numConn; i++){
        data = APP_PACK(serverIp, serverPort, i, buffer, dtLen, sem);
        pool.executeInsidePool(sendData, data);
        sem->wait();
    }

    pool.stop();
}
}
