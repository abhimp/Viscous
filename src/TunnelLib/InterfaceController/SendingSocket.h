/*
 * SendingSocket.h
 *
 *  Created on: 05-Apr-2017
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_INTERFACECONTROLLER_SENDINGSOCKET_H_
#define SRC_TUNNELLIB_INTERFACECONTROLLER_SENDINGSOCKET_H_

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

#include "../../util/ConditonalWait.hpp"
#include "../CommonHeaders.hpp"
#include <appThread.h>
#include "../PacketPool.h"

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

AppPool<SendMsgBuffer>& getSendMsgPool();

class SendingSocket {
private:
    int socket_descriptor;
    static std::mutex instantCreationMutex;
    static SendingSocket *instance;
    SendMsgBuffer *begQ, *endQ;
    appInt sizeQ;
    appBool sendingThreadRunning, stopSendingThread;
    std::mutex packeQueueLock;
    AppSemaphore waitForPacket;
    appThreadInfoId sendingTid;
    appInt32 packetCount;
    appInt64 byteCount;
    AppTimeCla appTime;

    SendingSocket();
    int init();
	SendMsgBuffer *getNextPacket();
	void addNextPacket(SendMsgBuffer *pkt);
	static void* startSendingInsideThread(void* data, appThreadInfoId tid);
	int sendMsgToSystemInterface(SendMsgBuffer *msg);
	void sendPacketFromQueue();
public:
	void sendMsg(SendMsgBuffer *msg);
    static SendingSocket * getInstance();
    virtual ~SendingSocket();
};

} /* namespace Interface */

#endif /* SRC_TUNNELLIB_INTERFACECONTROLLER_SENDINGSOCKET_H_ */
