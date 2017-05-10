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
