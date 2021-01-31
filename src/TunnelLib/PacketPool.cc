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
 * PacketPool.cpp
 *
 *  Created on: 13-Dec-2016
 *      Author: abhijit
 */

#include "PacketPool.hh"

#include <cstdlib>
#include <malloc.h>
//#define NO_POOL
PacketPool::PacketPool(appInt capa) :packetListing(NULL)
        , pool()
        , poolCapa(0)
        , poolTop(-1)
        , localPoolTop(0)
        , localPool(NULL)
        , rwlock()
        , circullarCnt(0)
        , totalPkt(0)
{

}

PacketPool::~PacketPool() {
    LOGE("Packets still in circulations: %ld", circullarCnt);
}

Packet *PacketPool::getNewPacket(appBool noNewData) {
    rwlock.lock();
    Packet *pkt;
#ifdef NO_POOL
    pkt = new Packet();
#else
#if 0
    if(pool.empty()){
        poolTop += 1;
        if(poolTop >= poolCapa){
            poolCapa = poolCapa==0 ? 2 : poolCapa*2;
            packetListing = (Packet **) appRealloc(packetListing, sizeof(Packet*)*poolCapa);
        }
        Packet *packets = new Packet[PACKETS_PER_POOL];
        APP_ASSERT(packets);
        packetListing[poolTop] = packets;
        for(appInt i = 0; i < PACKETS_PER_POOL; i++){
//            APP_ASSERT()
            pool.push(&packets[i]);
        }
    }
    pkt = pool.top();
    pool.pop();
#else
    if(localPool == NULL or localPoolTop == PACKETS_PER_POOL){
        if(pool.empty()){
            localPool = new Packet[PACKETS_PER_POOL];
            totalPkt += PACKETS_PER_POOL;
            APP_ASSERT(localPool);
            localPoolTop = 1;
            pkt = &localPool[0];
        }
        else{
            pkt = pool.top();
            pool.pop();
        }
    }
    else{
        pkt = &localPool[localPoolTop];
        localPoolTop++;
    }
#endif
    APP_ASSERT(pkt);
    if(noNewData && pkt->data){
        delete[] pkt->data;
        pkt->data = NULL;
    }
#endif //NO_POOL
    circullarCnt ++;
    rwlock.unlock();
    pkt->reInitHeader();
    return pkt;
}

Packet *PacketPool::getNewPacketWithData()
{
    Packet *pkt = getNewPacket(FALSE);
    pkt->reInitHeader();
    if(pkt->data == NULL){
        pkt->data = new appByte[MAX_PACKET_DATA_SIZE];
        pkt->capa = MAX_PACKET_DATA_SIZE;
    }
    return pkt;
}

void PacketPool::freePacket(Packet *pkt) {
    rwlock.lock();
    APP_ASSERT(pkt);
    freeOptionalHeaderToPool(pkt->optHeaders);
    pkt->optHeaders = NULL;
#ifdef __PROFILER_ENABLED__
    if (pkt->prof)
    	delete pkt->prof;
    pkt->prof = NULL;
    pkt->ticks = 0;
#endif
#ifndef NO_POOL
    pool.push(pkt);
#else
    if(pkt->data) delete[] pkt->data;
    delete pkt;
#endif
    circullarCnt --;
    rwlock.unlock();
}

inline void freeOptionalHeaderToPool(PacketOptionalAbstractHeader* pktHdr) {
    while(pktHdr){
        auto next = pktHdr->next;
        switch(pktHdr->type){
            case OPTIONAL_PACKET_HEADER_TYPE_ACK_HEADER:
                {
                    PacketAckHeader *pktAckHdr = (PacketAckHeader *)pktHdr;
                    getPacketAckedHeaderPool().free(pktAckHdr);
                }
                break;

            case OPTIONAL_PACKET_HEADER_TYPE_IP_ADDR:
                {
                    PacketIpHeader *pktIpHdr = (PacketIpHeader *)pktHdr;
                    delete pktIpHdr;
                }
                break;

            default:
                delete pktHdr;
//                APP_ASSERT(0 and "invalid header type");
        }
        pktHdr = next;
    }
}

PacketPool &getPacketPool(){
    static PacketPool packetPool;
    return packetPool;
}

util::AppPool<PacketAckHeader> &getPacketAckedHeaderPool(){
    static util::AppPool<PacketAckHeader> pakcetAckHeaderPool;
    return pakcetAckHeaderPool;
}
