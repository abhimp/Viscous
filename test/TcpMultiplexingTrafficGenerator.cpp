/*
 * TcpMultiplexingTrafficGenerator.cpp
 *
 *  Created on: 14-Apr-2017
 *      Author: abhijit
 */

#include "TcpMultiplexingTrafficGenerator.h"
#include <network.h>
#include <random>
#include "../src/util/AppStack.hpp"
#include "test_distribution.h"
#include "../src/util/ThreadPool.h"
#include  <appThread.h>
#include <iostream>

namespace TcpMultiplexing {
struct NFPacket{
    union {
        NFPacket *next;
        appInt64 data; //may not be used ever. just to make the field size equal accross devices
    };
    appInt16 flowIdex;
    appInt16 len;
    appBool close;
    appByte pkt[0];
};

struct NFPacketQueue{
    NFPacketQueue():success(FALSE), APP_LL_QUEUE_INIT_LIST(NFPacket){}
    AppSemaphore queueSem;
    AppSemaphore readySem;
    appBool success;

    APP_LL_QUEUE_DEFINE(NFPacket);
    APP_LL_QUEUE_ADD_FUNC(NFPacket);
    APP_LL_QUEUE_REMOVE_FUNC(NFPacket);
};

namespace Receiver {
}  // namespace Receiver

namespace Sender {

void *connectAndSend(void *data, appThreadInfoId tid){
    appByte *serverIp;
    appInt serverPort;
    NFPacketQueue *nfq;

    APP_UNPACK((appByte*)data, serverIp, serverPort, nfq);
    auto clientFd = app_tcp_client_connect_str(serverPort, (char *)serverIp);
    if(clientFd < 0){
        nfq->readySem.notify();
        return NULL;
    }
    nfq->success = TRUE;
    nfq->readySem.notify();

    while(1){
        nfq->queueSem.wait();
        auto pkt = nfq->getFromQueueNFPacket();
        if(!pkt) continue;
        if(pkt->close){
            appFreeWrapper(pkt);
            nfq->readySem.notify();
            break;
        }
        pkt->flowIdex = htons(pkt->flowIdex);
        pkt->len = htons(pkt->len);
        LOGE("tcp:sending: %d", pkt->flowIdex);
        send(clientFd, pkt, sizeof(NFPacket)+pkt->len, 0);
        appFreeWrapper(pkt);
    }
    close(clientFd);
    return NULL;
}

void *sendData(void *data){
    appInt index;
    appByte *dt;
    appInt dtLen;
    NFPacketQueue *nfq;
    APP_UNPACK((appByte *)data, index, dt, dtLen, nfq);
    auto count = staticDistribution::exponential2[index % (staticDistribution::exponential2Len)];
    std::cout << "starting " << index << std::endl;
    for(auto i = 0; i < count; i++){
        auto pkt = (NFPacket *)appCallocWrapper(1, sizeof(NFPacket) + dtLen);
        pkt->len = dtLen;
        pkt->flowIdex = index;
        memcpy(pkt->pkt, dt, dtLen);
        nfq->addToQueueNFPacket(pkt);
        nfq->queueSem.notify();
    }
    std::cout << "stoping " << index << std::endl;
    return NULL;
}

void startClient(appByte *serverIp, appInt serverPort, appInt numThread, appInt numConn){
    appByte *buffer;
    appByte dt[1290];
    appInt dtLen = sizeof(dt);
    NFPacketQueue nfqOb;
    NFPacketQueue *nfq = &nfqOb;
//    LOGE("starclient");
    for(appInt x = 0; x < sizeof(dt); x++){
        dt[x] = std::rand();
    }

    appByte *data;
    buffer = dt;

    UTIL::ThreadPool pool(numThread);
    data = APP_PACK(serverIp, serverPort, nfq);
    runInThreadGetTid(connectAndSend, data, FALSE, NULL);
    nfq->readySem.wait();

    if(nfq->success == FALSE) return;

    pool.run();
    if(nfq->success){
        LOGE("starclient");
        for(appInt i = 0; i < numConn; i++){
            data = APP_PACK(i, buffer, dtLen, nfq);
            pool.executeInsidePool(sendData, data);
        }
    }
    pool.stop();
    auto pkt = (NFPacket *)appCallocWrapper(1, sizeof(NFPacket) + dtLen);
    pkt->close = TRUE;
    nfq->addToQueueNFPacket(pkt);
    nfq->queueSem.notify();
    nfq->readySem.wait();
}
}  // namespace Sender

} /* namespace TcpMultiplexing */
