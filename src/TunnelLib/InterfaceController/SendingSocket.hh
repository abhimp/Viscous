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
 * SendingSocket.h
 *
 *  Created on: 05-Apr-2017
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_INTERFACECONTROLLER_SENDINGSOCKET_HH_
#define SRC_TUNNELLIB_INTERFACECONTROLLER_SENDINGSOCKET_HH_

//#include "../Packet.h"
#include <mutex>

#include <errno.h>            // errno, perror()
#include <netdb.h>            // struct addrinfo
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t, uint32_t
#include <sys/socket.h>       // needed for socket()
#include <netinet/in.h>       // IPPROTO_RAW, IPPROTO_UDP, INET_ADDRSTRLEN
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#include <netinet/udp.h>      // struct udphdr
#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <sys/ioctl.h>        // macro ioctl is defined
#include <bits/ioctls.h>      // defines values for argument "request" of ioctl.
#include <net/if.h>           // struct ifreq
#include <linux/if_ether.h>   // ETH_P_IP = 0x0800, ETH_P_IPV6 = 0x86DD
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <net/ethernet.h>

#include "../../util/ConditonalWait.hh"
#include "../CommonHeaders.hh"
#include <appThread.h>
#include "../PacketPool.hh"

#define MY_UDP_PKT_SIZE 2048

namespace Interface {

struct SendMsgBuffer{
    SendMsgBuffer():next(NULL), frame_len(0), deviceLen(0){}
    SendMsgBuffer *next;
    int frame_len;
    uint8_t ether_frame[MY_UDP_PKT_SIZE];
    sockaddr_ll device;
    int deviceLen;
};

util::AppPool<SendMsgBuffer>& getSendMsgPool();

class SendingSocket {
private:
    int socket_descriptor;
    static util::AppMutex instantCreationMutex;
    static SendingSocket *instance;
    SendMsgBuffer *begQ, *endQ;
    appInt sizeQ;
    appBool sendingThreadRunning, stopSendingThread;
    util::AppMutex packeQueueLock;
    util::AppSemaphore waitForPacket;
    appThreadInfoId sendingTid;
    appInt32 packetCount;
    appInt64 byteCount;
    AppTimeClass appTime;

    SendingSocket();
    int init();
    SendMsgBuffer *getNextPacket();
    void addNextPacket(SendMsgBuffer *pkt);
    static void* startSendingInsideThread(void* data, appThreadInfoId tid);
    int sendMsgToSystemInterface(SendMsgBuffer *msg);
    void sendPacketFromQueue();
public:
    void sendMsg(SendMsgBuffer *msg);
    int sendMsgDirectly(SendMsgBuffer *msg);
    static SendingSocket * getInstance();
    virtual ~SendingSocket();
};

} /* namespace Interface */

#endif /* SRC_TUNNELLIB_INTERFACECONTROLLER_SENDINGSOCKET_HH_ */
