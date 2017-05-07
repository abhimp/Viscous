/*
 * PacketPool.cpp
 *
 *  Created on: 13-Dec-2016
 *      Author: abhijit
 */

#include "PacketPool.h"
#include <cstdlib>
#include <malloc.h>
//#define NO_POOL
PacketPool::PacketPool(appInt capa) :packetListing(NULL), pool(), poolCapa(0), poolTop(-1), localPoolTop(0), localPool(NULL), rwlock(), circullarCnt(0)
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
                APP_ASSERT("invalid header type");
        }
        pktHdr = pktHdr->next;
    }
}

PacketPool &getPacketPool(){
    static PacketPool packetPool;
    return packetPool;
}

AppPool<PacketAckHeader> &getPacketAckedHeaderPool(){
    static AppPool<PacketAckHeader> pakcetAckHeaderPool;
    return pakcetAckHeaderPool;
}
