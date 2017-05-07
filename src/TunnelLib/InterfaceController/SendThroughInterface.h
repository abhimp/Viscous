/*
 * SendThroughInterface.h
 *
 *  Created on: 26-Nov-2016
 *      Author: abhijit
 */

#ifndef SENDTHROUGHINTERFACE_H_
#define SENDTHROUGHINTERFACE_H_
#include <stdio.h>
#include <unistd.h>           // close()
#include <cstring>           // strcpy, memset(), and memcpy()


#include <common.h>
#include "Addresses.hpp"
#include "../Packet.h"
#include <appThread.h>
#include "../../util/ConditonalWait.hpp"
#include <mutex>
#include "../PacketPool.h"
#include "SendingSocket.h"

// Define some constants.
#define IP4_HDRLEN 20         // IPv4 header length
#define UDP_HDRLEN  8         // UDP header length, excludes data
#define INTERFACE_SENDER_CNT 16


class SendThroughInterface {
public:
	SendThroughInterface(appString intf, ether_addr dst_addr, in_addr src_ip, appInt src_port);
	SendThroughInterface(appString intf, in_addr src_ip, appInt src_port);
	SendThroughInterface(InterfaceAddr *localAddr);
	virtual ~SendThroughInterface();
	int init();
	int sendPkt(Packet *pkt, in_addr dst_ip, appInt dst_port);
	in_addr getLocalIpNetwork();
private:
	inline uint16_t udp4_checksum (ip iphdr, udphdr udpheader, appString payload, appInt payloadlen);
	inline uint16_t checksum (uint16_t *addr, appInt len);
	inline void settingUpDestinationMac(ether_addr &destMac);
//	int sendPacket(appString data, appInt datalen, in_addr dst_ip, appInt dst_port);

//	uint8_t ether_frame[MY_UDP_PKT_SIZE];
	char interface[IF_NAMESIZE];
	in_addr src_ip, gw_ip;
	int ip_flags[4];
	ether_addr e_src_addr, e_dst_addr;
	sockaddr_ll device;
    ip iphdr;
    udphdr udpHeader;
    int src_port;
//    int socket_descriptor;
    appBool noDestMac;
	Interface::SendingSocket *opSock;
};

SendThroughInterface **getInterfaceSender();

InterfaceInfo **getInterfaceInfos();

#endif /* SENDTHROUGHINTERFACE_H_ */
