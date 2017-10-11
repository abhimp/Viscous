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
 * NewFlowHandler.hh
 *
 *  Created on: 10-Aug-2017
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_FLOWHANDLER_NEWFLOWHANDLER_HH_
#define SRC_TUNNELLIB_FLOWHANDLER_NEWFLOWHANDLER_HH_

#include <common.h>
#include <unistd.h>
#include <mutex>
#include <atomic>

#include "../../util/BitField.hh"
#include "../../util/ConditonalWait.hh"
#include "../CommonHeaders.hh"



namespace FlowHandler {

class NewFlowHandler : public ReliabilityMod {
public:
    virtual ~NewFlowHandler();
    NewFlowHandler(BaseReliableObj *parent, appFlowIdType flowId, appInt16 rcvWnd = 500);
    virtual appSInt recvPacketFrom(Packet *pkt, RecvSendFlags &flags);
    virtual appSInt readData(appFlowIdType flowId, appByte *data, appInt size);

    virtual appSInt sendPacketTo(appInt id, Packet *pkt);
    virtual appSInt sendData(appFlowIdType flowId, appByte *data, appInt dataLen);

    appInt16 getNextSeqNo();

    virtual appInt16 getReadUpto();
    virtual appStatus closeFlow(appFlowIdType flowId);

    void sendClosePacket();

    virtual appInt timeoutEvent(appTs time);
    virtual appSInt recvAck(appFlowIdType flowId, appInt16 flowSeqNo);
    virtual appStatus recvAck(PacketAckHeader *pktAckHdr);
    virtual void initiateClosure();
    virtual void setCallBack(EventType evt, void *dataToPass, void *func);
private:
    Packet **rcvPktQueue;

    appInt16 readUpto;
    appInt16 availableUpTo;
    util::AppMutex readLock;
    util::AppSemaphore readSem;

    appBool closeInitiated;
    appBool closed;
    util::AppMutex closeLock;
    appBool startedRecv;
    appBool startedSend;

    util::AppSemaphore remoteFreeWindow;
    util::BitField ackedBitField;
    appInt16 ackedUpto;

    appInt16 nextSeqNumber;
    util::AppSemaphore closeSemaphore;

    appInt16 recvWindow;

    inline appSInt sendPacketToCloseCheck(appInt id, Packet *pkt);
};

inline appSInt NewFlowHandler::sendPacketToCloseCheck(appInt id, Packet* pkt) {
    if(closeInitiated or closed)
        return -ERROR_FLOW_CLOSED;
    return sendPacketTo(id, pkt);
}

} /* namespace FlowHandler */

#endif /* SRC_TUNNELLIB_FLOWHANDLER_NEWFLOWHANDLER_HH_ */
