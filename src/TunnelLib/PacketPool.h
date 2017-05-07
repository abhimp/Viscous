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
#include "../../util/AppStack.hpp"
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
