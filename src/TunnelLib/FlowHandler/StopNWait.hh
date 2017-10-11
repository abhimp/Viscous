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
 * StopNWait.hh
 *
 *  Created on: 13-Aug-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_FlowHandler_STOPNWAIT_HPP_
#define SRC_TUNNELLIB_FlowHandler_STOPNWAIT_HPP_
#include "../../util/CircularBuffer.hh"
#include "../CommonHeaders.hh"
#ifdef __EXTREAME_DEBUG_COND
class debugData{
public:
    debugData():pkt(), data(NULL), len(0), seqNum(0), next(NULL){}
    Packet pkt;
    appByte *data;
    appInt len;
    appInt16 seqNum;
    debugData *next;
};
#endif

class StopNWait : public ReliabilityMod {
public:
    StopNWait(BaseReliableObj *parent, appInt16 flowId);

    virtual ~StopNWait();

    virtual appSInt sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen);
    virtual appSInt recvPacketFrom(Packet *pkt, struct sockaddr_in *src_addr, socklen_t addrlen);
    virtual appSInt readData(appInt16 flowId, appByte *data, appInt size);
    virtual appSInt sendData(appInt16 flowId, appByte *data, appInt dataLen);
    virtual appInt timeoutEvent(appTs time);
    virtual appStatus closeFlow(appInt16 flowId);
    void printStat();
//protected:
//    static void runInsideThread(void *data);

private:
    class SendBuffer {
    public:
        SendBuffer(appByte *data, appInt dlen, appInt16 seqNum, appBool fp) :
                seqNum(seqNum), firstPacket(fp), next(NULL), fin(FALSE) {
            if(data != NULL){
                this->data = new appByte[dlen];
                std::memcpy(this->data, data, dlen);
                this->dlen = dlen;
            }
            else{
                this->data = NULL;
                this->dlen = 0;
            }
        }
        ~SendBuffer(){ if(data) delete[] data; }
        appByte *data;
        appInt dlen;
        appInt16 seqNum;
        appBool firstPacket;
        SendBuffer *next;
        appBool fin;
        static const appInt maxDLen = 1300;
    };

//    appBool conClosed();
//    void conClosed(appBool);
//    inline appBool &conClosed(){return conClosed_;}
//    appStatus reset(appBool sendAck = FALSE);
    appStatus sendCloseAck();
    appInt16 getNextSeqNumber();
    appInt sendDataOrAckPacket(int pendingAck = 0);
//    void sendNotification(appInt jobId, appInt16 flowId, appBool insideThread = TRUE);

    util::CircularBuffer<appByte> receiveData;
    appInt bufferLen;
    appBool startedRecv;
    appBool firstPacket;
    appBool waitingForFinAck;
//    appBool waitingForFinACK, waitingForFinFINACK;
//    appBool conClosed_;
    appInt16 expectedSeq;
//    appInt16 expectedAck;
//    appInt16 pendingAck;
    appInt16 nextSeqNumber;
    SendBuffer *sndHead, *sndTail;
    pthread_mutex_t sendDataLock, sendPacketLock;
    sem_t sendBufferLock;
    appTs lastSent;
    appInt totalPktRcvd;
    appInt64 totalDataRcvd;
    appInt64 totalDataAcked;
#ifdef __EXTREAME_DEBUG_COND
    debugData *rcvHd, *rcvTl;
    debugData *sndHd, *sndTl;
#endif
};

#endif /* SRC_TUNNELLIB_FlowHandler_STOPNWAIT_HPP_ */
