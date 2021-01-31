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
 * Connection.cc
 *
 *  Created on: 07-Aug-2016
 *      Author: abhijit
 */

#include "Connection.hh"

#include "PacketPool.hh"

appSInt globalUDPSocketFD = 0;

inline appInt Connection::getLocalPort(){
    return ntohs(localAddr.sin_port);
}

appStatus Connection::startClient(appInt localPort, appByte *localIp){
    socketFd = app_udp_listener_str(localPort, (char *)localIp);
//    globalUDPSocketFD = socketFd;
    localAddrLen = sizeof(localAddr);
    getsockname(socketFd, (struct sockaddr*)&localAddr, &localAddrLen);
    LOGI("waiting for packet at %s:%d\n", inet_ntoa(localAddr.sin_addr), ntohs(localAddr.sin_port));

#ifdef APP_REC_HEADER
    int opt = 1;
    setsockopt(socketFd, IPPROTO_IP, IP_PKTINFO, &opt, sizeof(opt));
#endif

    APP_RETURN_SUCCESS;
}

appStatus Connection::startServer(appInt localPort, appByte *localIp){
    allowNewCon = TRUE;
    socketFd = app_udp_listener_str(localPort, (char *)localIp);
//    globalUDPSocketFD = socketFd;
    localAddrLen = sizeof(localAddr);
    getsockname(socketFd, (struct sockaddr*)&localAddr, &localAddrLen);
    LOGI("waiting for packet at port %s:%d\n", inet_ntoa(localAddr.sin_addr), ntohs(localAddr.sin_port));

    APP_RETURN_SUCCESS;
}


void Connection::listen() {
    appSInt n;
    appChar msg[2048];
    sockaddr_in src_addr, dest_addr;
    socklen_t len = sizeof(src_addr);

    while(1){
        n=recvfrom(socketFd,msg,sizeof(msg),0,(struct sockaddr*)&src_addr,&len);
        if(n == 0){
            LOGI("Socket colsed");
            ::close(socketFd);
            return;
        }
        if(n <= 0){
            perror("sum error");
            LOGI("Some unknown error. Received a data of len %d\n", n);
            continue;
        }

        Packet *pkt = decodeHeader((appByte *)msg, n);
        if(!pkt){
            LOGI("Error in header parsing\n");
            continue;
        }
        pkt->src_addr = src_addr;
        pkt->dest_addr = dest_addr;
        LOGI("pkt recvd: flowid:%d, fseq:%d, seq:%d ack:%d, sifc:%d, difc:%d", pkt->header.flowId, pkt->header.flowSeqNo, pkt->header.seqNo, pkt->header.ackNo, pkt->header.ifcsrc, pkt->header.ifcdst);
        RecvSendFlags flag = DEFALT_RECVSENDFLAG_VALUE;
        parent->recvPacketFrom(pkt, flag);
    }
}

appSInt Connection::sendPacketTo(appInt id, Packet *pkt){
    APP_ASSERT(0);
    return 0;
}

void Connection::close(void) {
    ::close(socketFd);
}

void Connection::force_close(void) {
    shutdown(socketFd, SHUT_RDWR);
}

Connection::Connection(BaseReliableObj* parent) : BaseReliableObj(parent)
        , localAddrLen(sizeof(localAddr))
        , socketFd(0)
        , allowNewCon(FALSE)
{
    std::memset(&localAddr, 0, sizeof(localAddr));
}
