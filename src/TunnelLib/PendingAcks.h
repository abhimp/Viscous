/*
 * PendingAcks.h
 *
 *  Created on: 15-Dec-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_PENDINGACKS_H_
#define SRC_TUNNELLIB_PENDINGACKS_H_
#include "Packet.h"
#include <map>
#include <mutex>

class PendingAcks {
public:
    PendingAcks();
    virtual ~PendingAcks();
    void addAck(PacketAckHeader *pktHdr);
    PacketAckHeader *removeAck();
    appBool haveAck();
private:
    PacketAckHeader **list;
    PacketAckHeader *begin; //I will use as stack. it wont be any problem for now as number of flow will be very small.
    appInt16 listCapa;
    std::map<appInt16, PacketAckHeader*> acks;
    std::mutex mtx;
};

#endif /* SRC_TUNNELLIB_PENDINGACKS_H_ */
