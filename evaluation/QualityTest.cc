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
 * QualityTest.cc
 *
 *  Created on: 19-Sep-2017
 *      Author: abhijit
 */

#include <cstdlib>
#include <ctime>
#include <iostream>

#include "QualityTest.hh"
#include "../src/TunnelLib/PacketPool.hh"
namespace Quality{


std::ostream& operator<<(std::ostream &os, ReceiverInfo &pktrd){
    os << pktrd.flowId << ":" << pktrd.readUpto;
    return os;
}

std::ostream& operator<<(std::ostream &os, PacketReadHeader &pktrd){
    for(auto x = pktrd.readerInfo; x; x = x->next)
        os << *x << std::endl;
    return os;
}

void addRandomeFlowInfo(PacketReadHeader *pkt){
    pkt->addInfo(rand(), rand());
}

PacketReadHeader *generateRandomReadheader(){
    auto pkt = GET_OPTIONAL_PACKET_HEADER_TYPE_READ_HDR;
    addRandomeFlowInfo(pkt);
    addRandomeFlowInfo(pkt);
    addRandomeFlowInfo(pkt);
    addRandomeFlowInfo(pkt);
    return pkt;
}

Packet *createPacket(){
    auto pkt = new Packet();
    auto x = generateRandomReadheader();
    pkt->optHeaders = x;
    std::cout << "Before encoding :" << std::endl;
    std::cout << *(x);
    return pkt;
}

void decodeNDisplay(Packet *pkt){
    appByte data[2048];
    auto len = encodeHeader(pkt, data, 2048);

    auto nptk = decodeHeader(data, len);
    PacketReadHeader *p = static_cast<PacketReadHeader*> (nptk->optHeaders);

    std::cout << "after encoding :" << std::endl;
    std::cout << *(p);
}

void testPacketWithReadData(){
    auto x = createPacket();
    decodeNDisplay(x);
}

}
