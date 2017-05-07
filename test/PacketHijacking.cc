/*
 * PacketHijacking.cc
 *
 *  Created on: 13-Apr-2017
 *      Author: abhijit
 */

#include "PacketHijacking.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <signal.h>
#include <time.h>
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#include <netinet/udp.h>      // struct udphdr
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <common.h>
#include <map>

#include "../src/TunnelLib/CommonHeaders.hpp"
#include "../src/util/Macros.h"
#include "../src/TunnelLib/ARQ/Streamer.h"
#include "../src/TunnelLib/ServerConnection.hpp"
#include "../src/TunnelLib/ClientConnection.hpp"
#include <appThread.h>

#define QUEUE_NUMBER 1
#define CATCH_SIG SIGUSR1

namespace PacketHijack{

void *startHijacking(void *arg, appThreadInfoId tid);//is con

struct FlowInfo{
    FlowInfo(): cCon(NULL), index(0), flowId(0), streamer(NULL), destIp(0), closed(FALSE){}
    ClientConnection *cCon;
    appSInt index;
    appInt16 flowId;
    ARQ::Streamer *streamer;
    appInt32 destIp;
    appBool closed;
};
struct NFPacket{
    NFPacket *next;
    appInt len;
    appByte pkt[0];
};

AppSemaphore queueSem;

APP_LL_QUEUE_DEFINE(NFPacket);
APP_LL_QUEUE_ADD_FUNC(NFPacket);
APP_LL_QUEUE_REMOVE_FUNC(NFPacket);

std::map<appInt32, FlowInfo *> destinationFlowMap;


void newDataToFlow(void *info, appByte *data, appInt dataLen){

}

void flowClosing(void *info){
    FlowInfo *finfo = (FlowInfo *)info;
    LOGI("testflowtrace2: %d, %ld", finfo->index, finfo->cCon->getTime().getMicro());
    destinationFlowMap.erase(finfo->destIp);
}

FlowInfo *getFinfo(appInt32 destIp, ClientConnection *cCon){
    if(hasKey(destinationFlowMap, destIp))
        return destinationFlowMap[destIp];

	FlowInfo *finfo = new FlowInfo();
	finfo->cCon = cCon;

	LOGI("testflowtrace2: %ld", cCon->getTime().getMicro())

	auto flow = cCon->addNewFlow();
	finfo->streamer = flow;
	finfo->flowId = flow->flowId();
	flow->setCallBack(ReliabilityMod::EVENT_NEW_DATA, finfo, (void *)newDataToFlow);
	flow->setCallBack(ReliabilityMod::EVENT_CLOSING, finfo, (void *)flowClosing);
	destinationFlowMap[destIp] = finfo;
	return finfo;
}

void sendDataToDestination(ClientConnection *cCon){

    while(1){
        queueSem.wait();
        auto pkt = getFromQueueNFPacket();
        struct iphdr *ipInfo = (struct iphdr *)pkt->pkt;
        appInt32 destIp = ntohl(ipInfo->daddr);
        auto finfo = getFinfo(destIp, cCon);
        finfo->streamer->sendData(pkt->pkt, pkt->len);
        appFreeWrapper(pkt);
    }
}

void startClient(appByte *serverIp, appInt serverPort, appInt queueNumber){

    APP_LL_QUEUE_RESET(NFPacket);
    ClientConnection cCon(serverIp, serverPort);
    void *self = &cCon;
    appByte *passData = APP_PACK(queueNumber, self);
    if(cCon.startClient() == APP_SUCCESS){
        runInThreadGetTid(startHijacking, passData, FALSE, NULL);
        sendDataToDestination(&cCon);
//        startSendingData(cCon, numThread, numPackets);
        cCon.close();
        cCon.waitToJoin();
    }
}

void sig_handler(int signo)
{
      if (signo != CATCH_SIG )
            return;
      printf("received SIG\n");
}

static int nfqCallbackFn(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data)
{
//    auto cCon = (ClientConnection *)data;
    u_int32_t id;
    int ret;
    appByte *ddata;
    time_t curtime;
    struct nfqnl_msg_packet_hdr *ph;
    time(&curtime);
    ret = nfq_get_payload(nfa, &ddata);
    ph = nfq_get_msg_packet_hdr(nfa);
    id = ntohl(ph->packet_id);
    struct iphdr *ipInfo = (struct iphdr *)ddata;
    if(ipInfo->protocol == IPPROTO_UDP or ret > 1290){
        struct udphdr *udpInfo = (struct udphdr *)(ddata + sizeof(struct iphdr));
        //printf("UDP: %s:%d => ", inet_ntoa(*((struct in_addr *)&ipInfo->saddr)), ntohs(udpInfo->source));
        //printf("%s:%d, ", inet_ntoa(*((struct in_addr *)&ipInfo->daddr)), ntohs(udpInfo->dest));
        //printf("UDP payload: %d\n", (int)(ret - sizeof(struct iphdr) - sizeof(struct udphdr)));
        auto pkt = (NFPacket *)appCallocWrapper(1, sizeof(NFPacket)+ret);
        memcpy(pkt->pkt, ddata, ret);
        pkt->len = ret;
        addToQueueNFPacket(pkt);
        queueSem.notify();
        goto REJECT_L;
    }
    else{
        printf("ip: %s => ", inet_ntoa(*((struct in_addr *)&ipInfo->saddr)));
        printf("%s,", inet_ntoa(*((struct in_addr *)&ipInfo->daddr)));
        printf("IP payload: %d\n", (int)(ret - sizeof(struct iphdr)));
        goto ACCEPTED_L;
    }
ACCEPTED_L:
    //printf("Packet Accepted - %d cur: %lu obj: %lu, accepted: %d, len: %d\n", ret, curtime, objTime, accepted, ret);
    return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
REJECT_L:
    //printf("Packet Rejected %d cur: %lu obj: %lu, accepted: %d, len: %d\n", ret, curtime, objTime, accepted, ret);
    return nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
}

void *startHijacking(void *arg, appThreadInfoId tid)//is con
{
    struct nfq_handle *nf_handler;
    struct nfq_q_handle *nf_q_handler;
    int nf_fd;
    int rcv_len;
    char buf[2048];
    appInt queueNumber;
    void *data;
    APP_UNPACK((appByte *)arg, queueNumber, data);

    if (signal(CATCH_SIG, sig_handler) == SIG_ERR){
          printf("\ncan't catch SIGINT\n");
          exit(3);
    }

    printf("opening library handle\n");
    nf_handler = nfq_open();
    if (!nf_handler) {
        fprintf(stderr, "error during nfq_open()\n");
        exit(1);
    }

    printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
    if (nfq_unbind_pf(nf_handler, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_unbind_pf()\n");
        exit(1);
    }

    printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
    if (nfq_bind_pf(nf_handler, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_bind_pf()\n");
        exit(1);
    }

    printf("binding this socket to queue '%d'\n", queueNumber);
    nf_q_handler = nfq_create_queue(nf_handler,  queueNumber, &nfqCallbackFn, data);
    if (!nf_q_handler) {
        fprintf(stderr, "error during nfq_create_queue()\n");
        exit(1);
    }

    printf("setting copy_packet mode\n");
    if (nfq_set_mode(nf_q_handler, NFQNL_COPY_PACKET, 0xffff) < 0) {
        fprintf(stderr, "can't set packet_copy mode\n");
        exit(1);
    }

    nf_fd = nfq_fd(nf_handler);

    // para el tema del loss:   while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0)

    while ((rcv_len = recv(nf_fd, buf, sizeof(buf), 0)))
    {
        //printf("pkt received\n");
        nfq_handle_packet(nf_handler, buf, rcv_len);
    }

    printf("unbinding from queue 0\n");
    nfq_destroy_queue(nf_q_handler);

#ifdef INSANE
    /* normally, applications SHOULD NOT issue this command, since
     * it detaches other programs/sockets from AF_INET, too ! */
    printf("unbinding from AF_INET\n");
    nfq_unbind_pf(nf_handler, AF_INET);
#endif

    printf("closing library handle\n");
    nfq_close(nf_handler);

    exit(0);
}

}


