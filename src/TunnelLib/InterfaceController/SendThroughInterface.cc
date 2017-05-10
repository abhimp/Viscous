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
 * SendThroughInterface.cpp
 *
 *  Created on: 26-Nov-2016
 *      Author: abhijit
 */

#include "../InterfaceController/SendThroughInterface.h"

#include <sstream>
#include <iostream>
#include "SearchMac.hpp"
#include "../PacketPool.h"


SendThroughInterface::SendThroughInterface(appString intf, ether_addr dst_addr, in_addr src_ip, appInt src_port):
	src_ip(src_ip), e_dst_addr(dst_addr), device(), src_port(src_port),
//	socket_descriptor(0),
	noDestMac(FALSE), opSock(NULL)
//	sendingTid(0), sendingThreadRunning(FALSE), stopSendingThread(FALSE), begQ(NULL), endQ(NULL), sizeQ(0)
{
	strcpy(interface, (char *)intf);
}

SendThroughInterface::SendThroughInterface(appString intf, in_addr src_ip, appInt src_port):
	src_ip(src_ip), device(), src_port(src_port),
//	socket_descriptor(0),
	noDestMac(TRUE), opSock(NULL)
//	sendingTid(0), sendingThreadRunning(FALSE), stopSendingThread(FALSE), begQ(NULL), endQ(NULL), sizeQ(0)
{
	strcpy(interface, (char *)intf);
}

SendThroughInterface::SendThroughInterface(InterfaceAddr *localAddr):
	src_ip(localAddr->ifcIp), gw_ip(localAddr->gwIp), e_dst_addr(localAddr->gwMac), device(),
	src_port(localAddr->localPort),
//	socket_descriptor(0),
	noDestMac(FALSE), opSock(NULL)//sendingTid(0), sendingThreadRunning(FALSE), stopSendingThread(FALSE), begQ(NULL), endQ(NULL), sizeQ(0)
{
	strcpy(interface, (char *)localAddr->ifc);
}

SendThroughInterface::~SendThroughInterface() {
}

in_addr SendThroughInterface::getLocalIpNetwork() {
    return src_ip;
}

void SendThroughInterface::settingUpDestinationMac(ether_addr &destMac) {
    std::memcpy (device.sll_addr, destMac.ether_addr_octet, 6);
    device.sll_halen = 6;
}

int SendThroughInterface::init(){

	int sd; //socket descriptor

	// Submit request for a socket descriptor to look up interface.
	if ((sd = socket (AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror ("socket() failed to get socket descriptor for using ioctl() ");
		return -1;
	}
	struct ifreq ifr;
	std::strcpy(ifr.ifr_ifrn.ifrn_name, interface);

	if (ioctl (sd, SIOCGIFHWADDR, &ifr) < 0) {
		perror ("ioctl() failed to get source MAC address");
		return 1;
	}
	close (sd);

    // Copy source MAC address.
    std::memcpy (e_src_addr.ether_addr_octet, ifr.ifr_hwaddr.sa_data, 6);

    // Find interface index from interface name and store index in
    // struct sockaddr_ll device, which will be used as an argument of sendto().
    if ((device.sll_ifindex = if_nametoindex (interface)) == 0) {
        perror ("if_nametoindex() failed to obtain interface index ");
        return 2;
    }

    device.sll_family = AF_PACKET;
    device.sll_protocol = htons (ETH_P_IP);
    if(!noDestMac)
        settingUpDestinationMac(e_dst_addr);


    //lets initializes ip hdr
    iphdr.ip_hl = IP4_HDRLEN / sizeof (uint32_t);

    // Internet Protocol version (4 bits): IPv4
    iphdr.ip_v = 4;

    // Type of service (8 bits)
    iphdr.ip_tos = 0;


    // ID sequence number (16 bits): unused, since single datagram
    iphdr.ip_id = htons (0);

    // Flags, and Fragmentation offset (3, 13 bits): 0 since single datagram

    // Zero (1 bit)
    ip_flags[0] = 0;

    // Do not fragment flag (1 bit)
    ip_flags[1] = 0;

    // More fragments following flag (1 bit)
    ip_flags[2] = 0;

    // Fragmentation offset (13 bits)
    ip_flags[3] = 0;

    iphdr.ip_off = htons ((ip_flags[0] << 15)
            + (ip_flags[1] << 14)
            + (ip_flags[2] << 13)
            +  ip_flags[3]);

    // Time-to-Live (8 bits): default to maximum value
    iphdr.ip_ttl = 255;

    // Transport layer protocol (8 bits): 17 for UDP
    iphdr.ip_p = IPPROTO_UDP;

    iphdr.ip_src = src_ip;

    // Source port number (16 bits): pick a number
    udpHeader.source = src_port;


    opSock = Interface::SendingSocket::getInstance();

	return 0;
}



// Computing the internet checksum (RFC 1071).
// Note that the internet checksum does not preclude collisions.
uint16_t SendThroughInterface::checksum (uint16_t *addr, appInt len)
{
    int count = len;
    register uint32_t sum = 0;
    sum = 0;
    uint16_t answer = 0;

    // Sum up 2-byte values until none or only one byte left.
    while (count > 1) {
        sum += *(addr++);
        count -= 2;
    }

    // Add left-over byte, if any.
    if (count > 0) {
        sum += *(uint8_t *) addr;
    }

    // Fold 32-bit sum into 16 bits; we lose information by doing this,
    // increasing the chances of a collision.
    // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    // Checksum is one's compliment of sum.
    answer = ~sum;

    return (answer);
}

// Build IPv4 UDP pseudo-header and call checksum function.
uint16_t SendThroughInterface::udp4_checksum (ip iphdr, udphdr udpheader, appString payload, appInt payloadlen)
{
    appChar buf[IP_MAXPACKET];
    appString ptr;
    appInt chksumlen = 0;
    appInt i;

    ptr = &buf[0];  // ptr points to beginning of buffer buf

    // Copy source IP address into buf (32 bits)
    memcpy (ptr, &iphdr.ip_src.s_addr, sizeof (iphdr.ip_src.s_addr));
    ptr += sizeof (iphdr.ip_src.s_addr);
    chksumlen += sizeof (iphdr.ip_src.s_addr);

    // Copy destination IP address into buf (32 bits)
    memcpy (ptr, &iphdr.ip_dst.s_addr, sizeof (iphdr.ip_dst.s_addr));
    ptr += sizeof (iphdr.ip_dst.s_addr);
    chksumlen += sizeof (iphdr.ip_dst.s_addr);

    // Copy zero field to buf (8 bits)
    *ptr = 0; ptr++;
    chksumlen += 1;

    // Copy transport layer protocol to buf (8 bits)
    memcpy (ptr, &iphdr.ip_p, sizeof (iphdr.ip_p));
    ptr += sizeof (iphdr.ip_p);
    chksumlen += sizeof (iphdr.ip_p);

    // Copy UDP length to buf (16 bits)
    memcpy (ptr, &udpheader.len, sizeof (udpheader.len));
    ptr += sizeof (udpheader.len);
    chksumlen += sizeof (udpheader.len);

    // Copy UDP source port to buf (16 bits)
    memcpy (ptr, &udpheader.source, sizeof (udpheader.source));
    ptr += sizeof (udpheader.source);
    chksumlen += sizeof (udpheader.source);

    // Copy UDP destination port to buf (16 bits)
    memcpy (ptr, &udpheader.dest, sizeof (udpheader.dest));
    ptr += sizeof (udpheader.dest);
    chksumlen += sizeof (udpheader.dest);

    // Copy UDP length again to buf (16 bits)
    memcpy (ptr, &udpheader.len, sizeof (udpheader.len));
    ptr += sizeof (udpheader.len);
    chksumlen += sizeof (udpheader.len);

    // Copy UDP checksum to buf (16 bits)
    // Zero, since we don't know it yet
    *ptr = 0; ptr++;
    *ptr = 0; ptr++;
    chksumlen += 2;

    // Copy payload to buf
    memcpy (ptr, payload, payloadlen);
    ptr += payloadlen;
    chksumlen += payloadlen;

    // Pad to the next 16-bit boundary
    for (i=0; i<payloadlen%2; i++, ptr++) {
        *ptr = 0;
        ptr++;
        chksumlen++;
    }

    return checksum ((uint16_t *) buf, chksumlen);
}

int SendThroughInterface::sendPkt(Packet* pkt, in_addr dst_ip,
        appInt dst_port) {

    if(noDestMac){
//        APP_ASSERT(0 && "Some problem in system");
        ether_addr destAddr;
        if(SearchMac::getMacR(interface, inet_ntoa(dst_ip), NULL, &destAddr)){
            exit(43);
        }
        settingUpDestinationMac(destAddr);
    }

    auto msg = Interface::getSendMsgPool().getNew();

    auto a_data = msg->ether_frame + IP4_HDRLEN + UDP_HDRLEN;
    auto a_datalen = encodeHeader(pkt, (appByte *)(a_data) , MY_UDP_PKT_SIZE - IP4_HDRLEN - UDP_HDRLEN);
    if(a_datalen == 0){
        Interface::getSendMsgPool().free(msg);
        return -1;
    }

    msg->frame_len = a_datalen + IP4_HDRLEN + UDP_HDRLEN;
    msg->device = device;
    msg->deviceLen = sizeof(device);

	// Total length of datagram (16 bits): IP header + UDP header + datalen
	iphdr.ip_len = htons (IP4_HDRLEN + UDP_HDRLEN + a_datalen);
	iphdr.ip_dst = dst_ip;
	iphdr.ip_sum = 0;
	iphdr.ip_sum = checksum ((uint16_t *) &iphdr, IP4_HDRLEN);

	// Destination port number (16 bits): pick a number
    udpHeader.dest = dst_port;

    // Length of UDP datagram (16 bits): UDP header + UDP data
    udpHeader.len = htons (UDP_HDRLEN + a_datalen);

    // UDP checksum (16 bits)
    udpHeader.check = udp4_checksum (iphdr, udpHeader, (appString)a_data, a_datalen);

    // Fill out ethernet frame header.

    // Ethernet frame length = ethernet data (IP header + UDP header + UDP data)
//    int frame_length = IP4_HDRLEN + UDP_HDRLEN + a_datalen;

    // IPv4 header
    memcpy (msg->ether_frame, &iphdr, IP4_HDRLEN);

    // UDP header
    memcpy (msg->ether_frame + IP4_HDRLEN, &udpHeader, UDP_HDRLEN);

    opSock->sendMsg(msg);

    return 0;
}

SendThroughInterface** getInterfaceSender() {
	static SendThroughInterface *interfaceSender[INTERFACE_SENDER_CNT] = {NULL}; //0 will never used
	return interfaceSender;
}

InterfaceInfo** getInterfaceInfos() {
    static InterfaceInfo *interfaceInfos[INTERFACE_SENDER_CNT] = {NULL}; //0 will never used
    return interfaceInfos;
}
