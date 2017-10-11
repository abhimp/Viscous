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
#include "Addresses.hh"
#include "../Packet.h"
#include <appThread.h>
#include "../../util/ConditonalWait.hh"
#include <mutex>

#include "../PacketPool.hh"
#include "SendingSocket.hh"

// Define some constants.
#define IP4_HDRLEN 20         // IPv4 header length
#define UDP_HDRLEN  8         // UDP header length, excludes data
#define INTERFACE_SENDER_CNT 16


class SendThroughInterface {
public:
//    SendThroughInterface(appString intf, ether_addr dst_addr, in_addr src_ip, appInt src_port);
    SendThroughInterface(appString intf, in_addr src_ip, appInt src_port);
    SendThroughInterface(InterfaceAddr *localAddr);
    virtual ~SendThroughInterface();
    int init();
    int sendPkt(Packet *pkt, in_addr dst_ip, appInt dst_port);
    in_addr getLocalIpNetwork();
    void getIface(appString iface, appInt len = 0);
    in_addr getLocIp(){return src_ip;}
    in_addr getGwIp(){return gw_ip;}
    ether_addr getLocMac(){return e_src_addr;}
    ether_addr getGwMac() {return e_dst_addr;}
private:
    inline uint16_t udp4_checksum (ip iphdr, udphdr udpheader, appString payload, appInt payloadlen);
    inline uint16_t checksum (uint16_t *addr, appInt len);
    inline void settingUpDestinationMac(ether_addr &destMac);
//    int sendPacket(appString data, appInt datalen, in_addr dst_ip, appInt dst_port);

//    uint8_t ether_frame[MY_UDP_PKT_SIZE];
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
