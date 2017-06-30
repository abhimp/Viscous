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
 * PacketPool.h
 *
 *  Created on: 13-Dec-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_PACKETPOOL_H_
#define SRC_TUNNELLIB_PACKETPOOL_H_
#include <stack>
#include <cstdlib>
#include "../../util/AppStack.hh"
#include "Packet.h"
#include <mutex>
#include "../util/AppPool.h"

class PacketPool {
public:
    PacketPool(appInt capa=0);
    virtual ~PacketPool();
    Packet *getNewPacket(appBool noNewData = FALSE);
    Packet *getNewPacketWithData();
    void freePacket(Packet *);
private:
    Packet **packetListing;
    AppStack<Packet *> pool;
    appSInt poolCapa;
    appSInt poolTop;
    appSInt localPoolTop;
    Packet *localPool;
    std::mutex rwlock;
    appSInt64 circullarCnt;
};


AppPool<PacketAckHeader> &getPacketAckedHeaderPool();

PacketPool &getPacketPool();

void freeOptionalHeaderToPool(PacketOptionalAbstractHeader* pktHdr);

#define GET_OPTIONAL_PACKET_HEADER_TYPE_ACK_HEADER getPacketAckedHeaderPool().getNew();
#define GET_OPTIONAL_PACKET_HEADER_TYPE_IP_ADDR new PacketIpHeader();
#endif /* SRC_TUNNELLIB_PACKETPOOL_H_ */
