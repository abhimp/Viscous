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
 * CubicChannelHandler.cc
 *
 *  Created on: 25-Dec-2016
 *      Author: sourav
 */

#include "CubicChannelHandler.hh"
#include "CongestionHandler.hh"
#include "../CommonHeaders.hh"
#include "../PacketPool.h"
#include <math.h>
//#include <linux/mm.h>
//#include <linux/module.h>
//#include <linux/math64.h>
//#include <linux/jiffies.h>
//#include <asm/param.h>
//#include <asm-generic/div64.h>
#include "../../util/LinuxMath.hh"
#include "log.hh"


#define HZ 1000
#define JIFFY_RATIO 1000
#define US_RATIO 1000000
#define MS_RATIO 1000


#define min(a,b) ((a >= b) ? b : a)
#define max(a,b) ((a <= b) ? b : a)
#define msecs_to_jiffies(x) ((JIFFY_RATIO/MS_RATIO)*(x))

//CubicChannelHandler::CubicChannelHandler(BaseReliableObj *parent,
//        InterfaceAddr &intfc, RemoteAddr &rmadr) :
//        ChannelHandler(parent, intfc, rmadr), advertisedWindow(1), rcvrWindow(
//                65535), seqNumberExpected(0), seqNumberRcvd(0), curr_ackno(0), CWND(
//                1), snd_cwnd_cnt(1), senderWindow(65535), RTT(1000), SRTT(0), RTTVAR(
//                0), RTO(3000), K(4), mulAlpha(7), scaleAlpha(8), mulBeta(3), scaleBeta(
//                4), ssthresh(65), fast_convergence(1), beta(717), bic_scale(41), tcp_friendliness(
//                1), cube_rtt_scale(0), beta_scale(0), cube_factor(0) {
//    cc_init();
//    firstTimeoutCalculation(RTT);
//}
//
//CubicChannelHandler::CubicChannelHandler(BaseReliableObj *parent,
//        InterfaceAddr &intfc, RemoteAddr &rmadr, appInt localPort) :
//        ChannelHandler(parent, intfc, rmadr, localPort), advertisedWindow(1), rcvrWindow(
//                65535), seqNumberExpected(0), seqNumberRcvd(0), curr_ackno(0), CWND(
//                1), snd_cwnd_cnt(1), senderWindow(65535), RTT(1000), SRTT(0), RTTVAR(
//                0), RTO(3000), K(4), mulAlpha(7), scaleAlpha(8), mulBeta(3), scaleBeta(
//                4), ssthresh(65), fast_convergence(1), beta(717), bic_scale(41), tcp_friendliness(
//                1), cube_rtt_scale(0), beta_scale(0), cube_factor(0) {
//    cc_init();
//    firstTimeoutCalculation(RTT);
//}

CubicChannelHandler::CubicChannelHandler(BaseReliableObj* parent,
        appInt8 ifcLoc, RemoteAddr& rmadr, appInt8 ifcSrc, appInt8 ifcDst) :
        ChannelHandler(parent, ifcLoc, rmadr, ifcSrc, ifcDst) ,advertisedWindow(1), rcvrWindow(
                65535), seqNumberExpected(0), seqNumberRcvd(0), curr_ackno(0), CWND(
                1),prev_CWND(0), snd_cwnd_cnt(1), senderWindow(65535), RTT(1000), SRTT(0), RTTVAR(
                0), RTO(3000), K(4), mulAlpha(7), scaleAlpha(8), mulBeta(3), scaleBeta(
                4), ssthresh(65), fast_convergence(1), beta(717), bic_scale(41), tcp_friendliness(
                1), cube_rtt_scale(0), beta_scale(0), cube_factor(0)
{
    cc_init();
    firstTimeoutCalculation(RTT);
}

CubicChannelHandler::~CubicChannelHandler() {
    // TODO Auto-generated destructor stub
}

appInt16 CubicChannelHandler::getCongestionWindowSize() {
    return CWND;
}

void CubicChannelHandler::setCongestionWindowSize(appInt16 windowSize) {
    CWND = windowSize;

}

appInt16 CubicChannelHandler::getSenderWindowSize() {
    return senderWindow;
}

void CubicChannelHandler::setSenderWindowSize() {
    senderWindow = min(CWND, advertisedWindow);

}

appInt16 CubicChannelHandler::getRcvrWindowSize() {
    return rcvrWindow;
}

void CubicChannelHandler::setRcvrWindowSize(appInt16 windowSize) {
    rcvrWindow = windowSize;

}

appSInt CubicChannelHandler::sendPacket(Packet *pkt) {
    /* Call this function till send_buffer is empty.*/
    LOG("Inside sendPacket\n");
    std::cout << "Inside sendPacket\n";
    LOG("send_buffer.lastByteSent: %d, send_buffer.lastByteAcked: %d, send_buffer.lastByteWritten: %d \n",
            send_buffer.lastByteSent, send_buffer.lastByteAcked, send_buffer.lastByteWritten);
    std::cout << send_buffer.lastByteSent << ":" << send_buffer.lastByteAcked
            << ":" << send_buffer.lastByteWritten << std::endl;
    if (send_buffer.lastByteAcked == -1 && send_buffer.lastByteWritten == 0
            && send_buffer.lastByteSent == 0
            && send_buffer.indicator == false) {
        write_mtx.lock();
        appInt16 byteToWrite = send_buffer.lastByteWritten;
        send_buffer.buffer[byteToWrite] = pkt;
        send_buffer.ACK_buffer[byteToWrite] = false;
        send_buffer.indicator = true;
        write_mtx.unlock();
        int n = send(send_buffer.buffer[byteToWrite]);
        if (n > 0) {
            send_buffer.Timeout_buffer[send_buffer.lastByteSent] =
                    getTime().getMili();
            LOG("Packet Sent: %d \n", send_buffer.buffer[byteToWrite]->header.seqNo);
            //std:: cout <<"1";
            //send_buffer.las
        }

        LOG("Exit sendPacket with return n\n");
        std::cout << "Exit sendPacket with return n\n";
        return n;

    } else {
        write_mtx.lock();
        bool flag = false;
        if (send_buffer.lastByteAcked == send_buffer.lastByteWritten)
            flag = true;
        appInt16 byteToWrite = (send_buffer.lastByteWritten + 1) % MAXSENDBUFFER;
        //LOG("Sequence Number : %d \n", byteToWrite);
        pkt->header.seqNo = byteToWrite;
        LOG("Packet Sequence Number : %d \n", pkt->header.seqNo);

        if(send_buffer.buffer[byteToWrite] == NULL)
            send_buffer.buffer[byteToWrite] = pkt;

        LOG("Buffer index and Packet Sequence Number and Packet sequence u=in buffer : %d , %d , %d \n",byteToWrite, pkt->header.seqNo,send_buffer.buffer[byteToWrite]->header.seqNo );
        send_buffer.ACK_buffer[byteToWrite] = false;
        send_buffer.lastByteWritten = byteToWrite;
        write_mtx.unlock();
        appInt16 byteToSend = (send_buffer.lastByteSent + 1) % MAXSENDBUFFER;
        if (send_buffer.buffer[byteToSend] != NULL && flag) {
            int n = send(send_buffer.buffer[byteToSend]);
            if (n > 0) {
                send_buffer.Timeout_buffer[byteToSend] = getTime().getMili();
                LOG("Packet Sent: %d \n", send_buffer.buffer[byteToSend]->header.seqNo);
            }
            send_buffer.lastByteSent = byteToSend;
            flag = false;

            LOG("Exit sendPacket with return 1\n");
            std::cout << "Exit sendPacket with return 1\n";

            return 1;
        }

    }
    LOG("Exit sendPacket with return -1\n");
    std::cout << "Exit sendPacket with return -1\n";
    return -1;

}

appSInt CubicChannelHandler::sendAck(appInt16 ackToSend, appInt16 fingerPrint,
        appInt16 flowId) {
    LOG("Inside sendAck\n");
    std::cout << "Inside sendAck\n";
    LOG(" ACK SENT: %d \n", ackToSend);
    std::cout << ackToSend << " ACK SENT \n";
    Packet *pkt = getPacketPool().getNewPacket();
    pkt->header.fingerPrint = fingerPrint;
    pkt->header.flowId = flowId;
    pkt->header.ackNo = ackToSend;
    pkt->header.window = rcvrWindow;
    pkt->header.flag = FLAG_ACK;

    LOG("Exit sendAck\n");
    std::cout << "Exit sendAck\n";
    return send(pkt);
}
/* checkValidAck:
 * 0 : Duplicate ACK received.
 * 1 : Valid ACK received.
 * 2 : Invalid ACK received.
 *
 * */

appInt8 CubicChannelHandler::checkValidAck(int ackno) {
    LOG("Inside checkValidAck\n");
    std::cout << "Inside checkValidAck\n";
    if (ackno == send_buffer.lastDuplicateAck) {
        send_buffer.countDuplicateAck++;
        LOG("Duplicate ack.\n");
        LOG("Exit checkValidAck : Duplicate ACK\n");
        std::cout << "Duplicate ack.\n";
        std::cout << "Exit checkValidAck : Duplicate ACK\n";
        return 0;
    } else if (send_buffer.lastByteAcked <= send_buffer.lastByteSent) {
        if (ackno
                > send_buffer.lastByteAcked&& ackno <= (send_buffer.lastByteSent + 1) % MAXSENDBUFFER) {
            send_buffer.lastDuplicateAck = ackno;
            send_buffer.countDuplicateAck = 0;
            LOG(" ack rcvd: %d \n", ackno);
            LOG("Exit checkValidAck : Valid ACK\n");
            std::cout << ackno << " ack rcvd.\n";
            std::cout << "Exit checkValidAck : Valid ACK\n";
            return 1;
        }
    } else if (send_buffer.lastByteAcked > send_buffer.lastByteSent) {
        if ((ackno > send_buffer.lastByteAcked && ackno < MAXSENDBUFFER)
                || (ackno >= 0
                        && ackno
                                <= (send_buffer.lastByteSent + 1)
                                        % MAXSENDBUFFER)) {
            send_buffer.lastDuplicateAck = ackno;
            send_buffer.countDuplicateAck = 0 ;
            LOG(" ack rcvd: %d \n", ackno);
            LOG("Exit checkValidAck : Valid ACK\n");
            std::cout << ackno << " ack rcvd.\n";
            std::cout << "Exit checkValidAck : Valid ACK\n";
            return 1;
        }
    }
    LOG(" invalid ack rcvd.\n");
    LOG("Exit checkValidAck : Invalid ACK\n");
    std::cout << " invalid ack rcvd.\n";
    std::cout << "Exit checkValidAck : Invalid ACK\n";
    return 2;

}

appSInt CubicChannelHandler::recv(Packet *pkt) {
    LOG("Inside recv\n");
    std::cout << "Inside recv\n";
    appSInt ret = -1;
    if (pkt->header.flag & FLAG_ACK) {
        std::cout << "Call  ackRecv\n";
        ret = ackRecv(pkt);
    }
    if ((pkt->header.flag & FLAG_DAT) && pkt->len > 0) {
        mtx_packet_rcv.lock();
        LOG("Call packet rcv for packet: %d \n", pkt->header.seqNo);
        std::cout << "Call packet rcv for packet: " << pkt->header.seqNo <<" \n";
        packetRcv(pkt);
        ret = 0;
        mtx_packet_rcv.unlock();
    }
    LOG("Exit recv\n");
    std::cout << "Exit recv\n";
    return ret;
}
appSInt CubicChannelHandler::ackRecv(Packet *pkt) {
    LOG("Inside ackRecv\n");
    std::cout << "Inside ackRecv\n";
    appInt16 ackno = pkt->header.ackNo;
    //LOG("Sequence Number in next Packet: %d , %d", ackno , send_buffer.buffer[ackno]->header.seqNo);
    appInt8 ackValidity = checkValidAck(ackno);
    //LOG("Sequence Number in next Packet: %d , %d", ackno , send_buffer.buffer[ackno]->header.seqNo);
    advertisedWindow = pkt->header.window;
    LOG("AckNo: %d, AckValidity: %d, countDuplicateAck: %d \n", ackno, ackValidity, send_buffer.countDuplicateAck);
    std::cout << "AckNo: " << ackno << "AckValidity: " << ackValidity
            << "countDuplicateAck: " << send_buffer.countDuplicateAck << "\n";
    if (ackValidity == 0 && send_buffer.countDuplicateAck == 1) {
        LOG("First duplicate ack rcvd: %d\n", ackno);
        std::cout << ackno << "First duplicate ack rcvd.\n";
        /*int range = (MAXSENDBUFFER + (ackno - send_buffer.lastByteAcked -1))
         % MAXSENDBUFFER;
         for (int i = 1; i <= range; i++) {
         send_buffer.ACK_buffer[(i + send_buffer.lastByteAcked)
         % MAXSENDBUFFER] = true;
         send_buffer.buffer[i] = NULL;
         send_buffer.Timeout_buffer[i].reInit();
         }
         int countByteToSend = getSenderWindowSize() - range;
         for (int i = 1; i < countByteToSend; i++) {
         appInt16 byteToSend = (send_buffer.lastByteSent + i) % MAXSENDBUFFER;
         if (send_buffer.buffer[byteToSend] != NULL) {
         int n = send(send_buffer.buffer[byteToSend]);
         if (n > 0) {
         send_buffer.Timeout_buffer[byteToSend] = getTime().getMili();
         send_buffer.lastByteSent = byteToSend;
         }
         }
         }
         return 1;*/


    } else if (ackValidity == 0 && send_buffer.countDuplicateAck == 2) {
        LOG("Second duplicate ack rcvd: %d \n", ackno);
        std::cout << ackno << "Second duplicate ack rcvd.\n";
        /*int range = (MAXSENDBUFFER + (ackno - send_buffer.lastByteAcked -1))
         % MAXSENDBUFFER;
         for (int i = 1; i <= range; i++) {
         send_buffer.ACK_buffer[(i + send_buffer.lastByteAcked)
         % MAXSENDBUFFER] = true;
         send_buffer.buffer[i] = NULL;
         send_buffer.Timeout_buffer[i].reInit();
         }
         int countByteToSend = getSenderWindowSize() - range;
         for (int i = 1; i < countByteToSend; i++) {
         appInt16 byteToSend = (send_buffer.lastByteSent + i) % MAXSENDBUFFER;
         if (send_buffer.buffer[byteToSend] != NULL) {
         int n = send(send_buffer.buffer[byteToSend]);
         if (n > 0) {
         send_buffer.Timeout_buffer[byteToSend] = getTime().getMili();
         send_buffer.lastByteSent = byteToSend;

         }
         }
         }
         return 1;*/

    } else if (ackValidity == 0 && send_buffer.countDuplicateAck == 3) {
        LOG("Third duplicate ack rcvd: %d \n", ackno);
        LOG("%d packet will be send \n", send_buffer.lastDuplicateAck);
        std::cout << ackno << "Third duplicate ack rcvd.\n";
        appInt16 byteToSend = send_buffer.lastDuplicateAck;
        //advertisedWindow = pkt->header.window;

        if (send_buffer.buffer[byteToSend] != NULL) {
            int n = send(send_buffer.buffer[byteToSend]);
            if (n > 0) {
                send_buffer.Timeout_buffer[byteToSend] = getTime().getMili();
                packet_loss();
                setSenderWindowSize();
                LOG("Exit ackRecv : 3 Duplicate ACK: Packet Sent.\n");
                std::cout << "Exit ackRecv : 3 Duplicate ACK: Packet Sent.\n";
                return 1;
            }
        }
        LOG("Exit ackRecv : 3 Duplicate ACK\n");
        std::cout << "Exit ackRecv : 3 Duplicate ACK\n";

    } else if (ackValidity == 1 && send_buffer.countDuplicateAck == 0) {
        LOG("normal ack rcvd: %d \n", ackno);
        std::cout << ackno << "normal ack rcvd.\n";
        int range = (MAXSENDBUFFER + (ackno - send_buffer.lastByteAcked - 1))
                % MAXSENDBUFFER;
        for (int i = 1; i <= range; i++) {
            send_buffer.lastByteAcked = (1 + send_buffer.lastByteAcked)
                    % MAXSENDBUFFER;
            send_buffer.ACK_buffer[send_buffer.lastByteAcked] = true;
            auto localPkt = send_buffer.buffer[send_buffer.lastByteAcked];
            freePacket(localPkt);
            send_buffer.buffer[send_buffer.lastByteAcked] = NULL;
            send_buffer.Timeout_buffer[send_buffer.lastByteAcked] = -1;

        }
        //advertisedWindow = pkt->header.window;

        LOG("advWindow: %d, CWND: %d \n", advertisedWindow, CWND);
        std::cout << "advWindow:" << advertisedWindow << ": CWND :" << CWND << std::endl;
        LOG("Current TIme: %l, buffer Time: %l \n", getTime().getMili(), send_buffer.Timeout_buffer[(ackno-1) % MAXSENDBUFFER]);
        timeoutCalculation(
                getTime().getMili() - send_buffer.Timeout_buffer[(ackno-1) % MAXSENDBUFFER]);

        ack_rcv(range);
        setSenderWindowSize();
        LOG("prev_CWND: %d , SenderWindow: %d, Range: %d",prev_CWND , getSenderWindowSize(), range);
        std::cout << "SenderWindow: " << getSenderWindowSize() << ":" << range
                << "\n";
        //int countByteToSend = getSenderWindowSize() - range;
        int countByteToSend = getSenderWindowSize() - prev_CWND + range;
        if(countByteToSend <=0) //Delete this code, it is for testing purpose.
                    countByteToSend=4;
        LOG("countByteToSend: %d", countByteToSend);
        prev_CWND = getSenderWindowSize();
        for (int i = 1; i <= countByteToSend; i++) {
            appInt16 byteToSend = (send_buffer.lastByteSent + 1) % MAXSENDBUFFER;
            //LOG("Sequence Number to Send : %d \n", byteToSend);
            //LOG("Packet Sequence Number to Send : %d \n", send_buffer.buffer[byteToSend]->header.seqNo);
            if (send_buffer.buffer[byteToSend] != NULL) {
                //LOG("Inside if Packet Sequence Number to Send : %d \n", send_buffer.buffer[byteToSend]->header.seqNo);
                int n = send(send_buffer.buffer[byteToSend]);
                if (n > 0) {
                    send_buffer.Timeout_buffer[byteToSend] =
                            getTime().getMili();
                    send_buffer.lastByteSent = byteToSend;
                }
            }
        }
        LOG("Exit ackRecv : Valid ACK\n");
        std::cout << "Exit ackRecv : Valid ACK\n";
        return 1;
    }
    LOG("Exit ackRecv : Invalid ACK\n");
    std::cout << "Exit ackRecv : Invalid ACK\n";
    return -1;
}

int CubicChannelHandler::checkValidPacket(int expectedSeq, int lastRcvdSeq,
        bool indicatorNextByteExpected, bool indicatorLastByteRcvd,
        int curSeq) {
    LOG("Inside checkValidPacket\n");
    std::cout << "Inside checkValidPacket\n";
    int seqDiff;
    if (indicatorNextByteExpected != indicatorLastByteRcvd) {
        seqDiff = expectedSeq - lastRcvdSeq;
        seqDiff = seqDiff / 2;
        if (curSeq > lastRcvdSeq && curSeq < (lastRcvdSeq + seqDiff))
            return 2;
        else if (curSeq >= (lastRcvdSeq + seqDiff) && curSeq < expectedSeq)
            return 0;
        else
            return 1;
    } else {
        seqDiff = MAXRCVBUFFER - (lastRcvdSeq - expectedSeq);
        seqDiff = seqDiff / 2;
        if (((lastRcvdSeq + seqDiff) % MAXRCVBUFFER) > lastRcvdSeq)/*Then both are in 0 side*/
        {
            if (curSeq >= expectedSeq && curSeq < lastRcvdSeq)
                return 1;
            else if (curSeq > lastRcvdSeq && curSeq < (lastRcvdSeq + seqDiff))
                return 2;
            else
                return 0;

        } else /*Both are in 65535 side*/
        {
            if (curSeq >= expectedSeq && curSeq < lastRcvdSeq)
                return 1;
            else if (curSeq < expectedSeq && curSeq > (expectedSeq - seqDiff))
                return 0;
            else
                return 2;
        }
    }

}

bool CubicChannelHandler::isOrdered(int nextByteExpected, int lastByteRcvd) {

    if (((lastByteRcvd + 1) % MAXRCVBUFFER) == nextByteExpected)
        return true;
    else
        return false;

}

void CubicChannelHandler::packetRcv(Packet *pkt) {
    mtx_rcv_buffer.lock();
    LOG("Inside packetRcv\n");
    std::cout << "Inside packetRcv\n";
    int diffRcvd;
    appInt16 fingerPrint, flowId;
    int seqNo = pkt->header.seqNo;
    fingerPrint = pkt->header.fingerPrint;
    flowId = pkt->header.flowId;
    LOG("Sequence Number: %d \n", seqNo);
    LOG("seqNumberExpected: %d \n", seqNumberExpected);
    std::cout << seqNo << "\n";
    if (rcv_buffer.lastByteRead == 0 && rcv_buffer.lastByteRcvd == 0
            && rcv_buffer.lastByteRcvd == 0 && rcv_buffer.indicator == 0)/*First Packet Received.*/
            {

        rcv_buffer.indicator = 1;
        rcv_buffer.buffer[rcv_buffer.lastByteRcvd] = pkt->header.seqNo;
        rcv_buffer.nextByteExpected = rcv_buffer.lastByteRcvd + 1;
        seqNumberExpected = (seqNo + 1) % MAXRCVBUFFER;
        LOG("seqNumberExpected: %d, nextByteExpected: %d, lastByteRcvd: %d \n",seqNumberExpected, rcv_buffer.nextByteExpected,rcv_buffer.lastByteRcvd);
        std::cout << "nextByteExpected: " << rcv_buffer.nextByteExpected
                << "lastByteRcvd: " << rcv_buffer.lastByteRcvd << "\n";
        rcvrWindow = (MAXRCVBUFFER
                + (rcv_buffer.lastByteRcvd - rcv_buffer.nextByteExpected))
                % MAXRCVBUFFER; // As it is the initial case.

        seqNumberRcvd = seqNo % MAXRCVBUFFER;
        curr_ackno = (seqNumberRcvd + 1) % MAXRCVBUFFER;

    } else {

        diffRcvd = (pkt->header.seqNo - seqNumberRcvd);
        /*When received packet is next packet of lastByteRcvd */
        if (diffRcvd == 1 || diffRcvd == -65535) {
            if ((rcv_buffer.lastByteRcvd + 1) % MAXRCVBUFFER
                    == rcv_buffer.nextByteExpected) {/*If nextByteExpected is the next packet of lastByteRcvd.*/
                int nextValue = (rcv_buffer.nextByteExpected + 1) % MAXRCVBUFFER;
                if (rcv_buffer.nextByteExpected > nextValue) {
                    rcv_buffer.indicatorNextByteExpected = 1; /*Side Changes due to modulus*/

                    /*If all indicators are in same side reset them all.*/
                    if ((rcv_buffer.indicatorLastByteRcvd == 1)
                            && (rcv_buffer.indicatorNextByteExpected == 1)
                            && (rcv_buffer.indicatorLastByteRead == 1)) {
                        rcv_buffer.indicatorLastByteRcvd = 0;
                        rcv_buffer.indicatorNextByteExpected = 0;
                        rcv_buffer.indicatorLastByteRead = 0;
                    }
                }

                rcv_buffer.nextByteExpected = nextValue;
                curr_ackno = nextValue;
                seqNumberExpected = (seqNo + 1) % MAXRCVBUFFER;
            }

            int nextValue = (rcv_buffer.lastByteRcvd + 1) % MAXRCVBUFFER;
            if (rcv_buffer.lastByteRcvd > nextValue) {
                rcv_buffer.indicatorLastByteRcvd = 1; /*Side Changes due to modulus*/

                /*If all indicators are in same side reset them all.*/
                if ((rcv_buffer.indicatorLastByteRcvd == 1)
                        && (rcv_buffer.indicatorNextByteExpected == 1)
                        && (rcv_buffer.indicatorLastByteRead == 1)) {
                    rcv_buffer.indicatorLastByteRcvd = 0;
                    rcv_buffer.indicatorNextByteExpected = 0;
                    rcv_buffer.indicatorLastByteRead = 0;
                }
            }

            rcv_buffer.lastByteRcvd = nextValue;
            seqNumberRcvd = seqNo;
            rcv_buffer.buffer[rcv_buffer.lastByteRcvd] = seqNo;
            LOG("seqNumberExpected: %d, nextByteExpected: %d, lastByteRcvd: %d \n",seqNumberExpected, rcv_buffer.nextByteExpected,rcv_buffer.lastByteRcvd);
            std::cout << "nextByteExpected: " << rcv_buffer.nextByteExpected
                    << "lastByteRcvd: " << rcv_buffer.lastByteRcvd << "\n";
            if (isOrdered(rcv_buffer.nextByteExpected,
                    rcv_buffer.lastByteRcvd)) {
                rcvrWindow = (MAXRCVBUFFER - 1);
            } else {
                rcvrWindow =
                        (MAXRCVBUFFER
                                + (rcv_buffer.nextByteExpected
                                        - rcv_buffer.lastByteRcvd))
                                % MAXRCVBUFFER;
            }

        } else {
            /*Received packet is the nextByteExpected Packet.*/
            if (seqNumberExpected == seqNo) {
                int nextValue = (rcv_buffer.nextByteExpected + 1) % MAXRCVBUFFER;
                if (rcv_buffer.nextByteExpected > nextValue) {
                    rcv_buffer.indicatorNextByteExpected = 1; /*Side Changes due to modulus*/
                    if ((rcv_buffer.indicatorLastByteRcvd == 1)
                            && (rcv_buffer.indicatorNextByteExpected == 1)
                            && (rcv_buffer.indicatorLastByteRead == 1)) {
                        rcv_buffer.indicatorLastByteRcvd = 0;
                        rcv_buffer.indicatorNextByteExpected = 0;
                        rcv_buffer.indicatorLastByteRead = 0;
                    }
                }
                rcv_buffer.buffer[rcv_buffer.nextByteExpected] = seqNo;
                rcv_buffer.nextByteExpected = nextValue;
                curr_ackno = nextValue;
                seqNumberExpected = (seqNo + 1) % MAXRCVBUFFER;

                while (rcv_buffer.buffer[rcv_buffer.nextByteExpected] != 0) {
                    int nextValue = (rcv_buffer.nextByteExpected + 1)
                            % MAXRCVBUFFER;
                    if (rcv_buffer.nextByteExpected > nextValue) {
                        rcv_buffer.indicatorNextByteExpected = 1; /*Side Changes due to modulus*/
                        if ((rcv_buffer.indicatorLastByteRcvd == 1)
                                && (rcv_buffer.indicatorNextByteExpected == 1)
                                && (rcv_buffer.indicatorLastByteRead == 1)) {
                            rcv_buffer.indicatorLastByteRcvd = 0;
                            rcv_buffer.indicatorNextByteExpected = 0;
                            rcv_buffer.indicatorLastByteRead = 0;
                        }
                    }
                    rcv_buffer.nextByteExpected = nextValue;
                    curr_ackno = nextValue;
                    seqNumberExpected = (seqNo + 1) % MAXRCVBUFFER;

                    //rcv_buffer.lastByteRcvd = (rcv_buffer.lastByteRcvd +1) % MAXRCVBUFFER;
                    //seqNumberRcvd = rcvd_packet->sequence;
                }

            } else {/*Any other scenario for received packet.*/
                int packetStatus = checkValidPacket(
                        rcv_buffer.buffer[(rcv_buffer.nextByteExpected - 1) %
                        MAXRCVBUFFER],
                        rcv_buffer.buffer[rcv_buffer.lastByteRcvd],
                        rcv_buffer.indicatorNextByteExpected,
                        rcv_buffer.indicatorLastByteRcvd, seqNo);

                if (packetStatus == 1) {/*Packet is in between nextByteExpected and lastByteReceived.*/
                    /*int distance =
                     (MAXRCVBUFFER
                     + (seqNo
                     - rcv_buffer.buffer[(rcv_buffer.nextByteExpected
                     - 1)])) % MAXRCVBUFFER;
                     rcv_buffer.buffer[(rcv_buffer.nextByteExpected
                     + (distance - 1)) % MAXRCVBUFFER] = seqNo;*/
                    rcv_buffer.buffer[seqNo % MAXRCVBUFFER] = seqNo
                            % MAXRCVBUFFER;

                } else if (packetStatus == 2) {/*Packet is after lastByteReceived.*/
                    /*int distance =
                     (MAXRCVBUFFER
                     + (seqNo
                     - rcv_buffer.buffer[rcv_buffer.lastByteRcvd]))
                     % MAXRCVBUFFER;
                     rcv_buffer.buffer[(rcv_buffer.lastByteRcvd + (distance - 1))
                     % MAXRCVBUFFER] = seqNo;*/
                    int nextValue = seqNo % MAXRCVBUFFER;
                    rcv_buffer.buffer[nextValue] = nextValue;
                    //int nextValue = (rcv_buffer.lastByteRcvd + distance) % MAXRCVBUFFER;

                    if (nextValue < rcv_buffer.lastByteRcvd) {
                        rcv_buffer.indicatorLastByteRcvd = 1;
                        if ((rcv_buffer.indicatorLastByteRcvd == 1)
                                && (rcv_buffer.indicatorNextByteExpected == 1)
                                && (rcv_buffer.indicatorLastByteRead == 1)) {
                            rcv_buffer.indicatorLastByteRcvd = 0;
                            rcv_buffer.indicatorNextByteExpected = 0;
                            rcv_buffer.indicatorLastByteRead = 0;
                        }
                    }
                    rcv_buffer.lastByteRcvd = nextValue;
                    seqNumberRcvd = seqNo;
                }

            }
        }
        LOG("seqNumberExpected: %d, nextByteExpected: %d, lastByteRcvd: %d \n",seqNumberExpected, rcv_buffer.nextByteExpected,rcv_buffer.lastByteRcvd);
        std::cout << "nextByteExpected: " << rcv_buffer.nextByteExpected
                << "lastByteRcvd: " << rcv_buffer.lastByteRcvd << "\n";
        if (isOrdered(rcv_buffer.nextByteExpected, rcv_buffer.lastByteRcvd)) {
            rcvrWindow = (MAXRCVBUFFER - 1);
        } else {
            rcvrWindow = (MAXRCVBUFFER
                    + (rcv_buffer.nextByteExpected - rcv_buffer.lastByteRcvd))
                    % MAXRCVBUFFER;
        }


    }

    mtx_rcv_buffer.unlock();
    //setRcvrWindowSize(rcvrWindow);
    sendAck(curr_ackno, fingerPrint, flowId);

    LOG("Exit packetRcv\n");
    std::cout << "Exit packetRcv\n";
}

appInt CubicChannelHandler::timeoutEvent(appTs time) {
    appSInt64 cur_time = time.getMili();
    int RTOByte = send_buffer.lastByteSent;
    if (send_buffer.Timeout_buffer[RTOByte] != -1
            && send_buffer.indicator == true) {
        if (cur_time >= (send_buffer.Timeout_buffer[RTOByte] + RTO)) {
            int n = send(send_buffer.buffer[RTOByte]);
            if (n > 0) {
                LOG("Inside timeoutEvent: Timeout Occured.\n");
                std::cout << "Inside timeoutEvent: Timeout Occured.\n";
                send_buffer.Timeout_buffer[RTOByte] = getTime().getMili();
                cc_reset();
            }
        }
    }
    RTOByte = (RTOByte + 1) % MAXSENDBUFFER;
    while (send_buffer.Timeout_buffer[RTOByte] != -1
            && cur_time >= (send_buffer.Timeout_buffer[RTOByte] + RTO)) {
        int n = send(send_buffer.buffer[RTOByte]);
        if (n > 0) {
            send_buffer.Timeout_buffer[RTOByte] = getTime().getMili();
            cc_reset();
            RTOByte = (RTOByte + 1) % MAXSENDBUFFER;
        }
    }
    return 0;
}

/*Function for CUBIC Congestion Controller*/

void CubicChannelHandler::cc_init() {
    LOG("Inside cc_init \n");
    LOG("Before Update: \n");
    LOG("beta_scale: %d, cube_rtt_scale: %d, cube_factor: %l \n", beta_scale, cube_rtt_scale, cube_factor);

    beta_scale = 8 * (BICTCP_BETA_SCALE + beta) / 3
            / (BICTCP_BETA_SCALE - beta);

    cube_rtt_scale = (bic_scale * 10);

    cube_factor = 1ull << (10 + 3 * BICTCP_HZ); /* 2^40 */

    do_div(cube_factor, bic_scale * 10);

    LOG("After Update: \n");
    LOG("beta_scale: %d, cube_rtt_scale: %d, cube_factor: %l \n", beta_scale, cube_rtt_scale, cube_factor);

    LOG("Exit cc_init \n");
}

void CubicChannelHandler::cc_reset() {
    ca.cnt = 0;
    ca.last_max_cwnd = 0; /*W_last_max*/
    ca.loss_cwnd = 0;
    ca.last_cwnd = 0;
    ca.last_time = 0;
    ca.origin_point = 0;
    ca.K = 0;
    ca.delay_min = 0; /*dMin*/
    ca.epoch_start = 0;
    ca.ack_cnt = 0;
    ca.tcp_cwnd = 0; /*W_tcp*/
    ca.delayed_ack = 2 << ACK_RATIO_SHIFT;

}

void CubicChannelHandler::cc_friendliness(appInt32 cwnd) {
    appInt32 max_cnt = 0;
    ca.tcp_cwnd = ca.tcp_cwnd + (3 * beta) / (3 - beta) + ca.ack_cnt / cwnd;
    ca.ack_cnt = 0;

    if (ca.tcp_cwnd > cwnd) {
        max_cnt = cwnd / (ca.tcp_cwnd - cwnd);
        std::cout << "max_cnt" << max_cnt << std::endl;
        if (ca.cnt > max_cnt)
            ca.cnt = max_cnt;
        std::cout << "ca-cnt" << ca.cnt << std::endl;
    }
}

void CubicChannelHandler::cc_update(appInt32 cwnd, uint32_t acked) {
    LOG("Inside cc_update \n");
    appInt32 delta, target, max_cnt, min_cnt;
    appInt64 offs, t;
    LOG("CWND: %d", cwnd);
    std::cout << "cc_update:cwnd: " << cwnd << std::endl;
    ca.ack_cnt = ca.ack_cnt + acked;

    /*Uncomment this code if tcp_time_stamp works*/
    //if(ca.last_cwnd == cwnd && (appSInt32)(tcp_time_stamp - ca.last_time) <= HZ/32)
    //return;

    ca.last_cwnd = cwnd;
    ca.last_time = getTime().getMili();//ca.last_time = tcp_time_stamp;

    if (ca.epoch_start == 0) {
        ca.epoch_start = getTime().getMili(); //ca.epoch_start = tcp_time_stamp;
        ca.ack_cnt = acked;
        ca.tcp_cwnd = cwnd;
        if (cwnd < ca.last_max_cwnd) {
            ca.K = cbrt((ca.last_max_cwnd - cwnd) * cube_factor); /*replace cbrt by cubic_root*/
            ca.origin_point = ca.last_max_cwnd;
            LOG("ca.last_max_cwnd: %d \n", ca.last_max_cwnd);
            std::cout << "cc_update:ca.last_max_cwnd: " << ca.last_max_cwnd
                    << std::endl;
        } else {
            ca.K = 0;
            ca.origin_point = cwnd;
        }
    }
    t = (appSInt32)(getTime().getMili() - ca.epoch_start);
    t = t + msecs_to_jiffies(ca.delay_min >> 3);
    t <<= BICTCP_HZ;
    t = t/HZ;
    //do_div(t, HZ);
    LOG("ca.delay_min: %d, t: %l, ca.K: %d \n",ca.delay_min, t, ca.K );
    std::cout << "cc_update:ca.delay_min: " << ca.delay_min << std::endl;
    std::cout << "cc_update:t: " << t << std::endl;
    std::cout << "cc_update:ca.K: " << ca.K << std::endl;
    //t = t + msecs_to_jiffies(ca.delay_min >> 3);
    //t <<= BICTCP_HZ;
    //do_div(t, HZ);
    if (t < ca.K)
        offs = ca.K - t;
    else
        offs = t - ca.K;
    LOG("offs: %l \n", offs);
    std::cout << "cc_update:offs: " << offs << std::endl;
    delta = (cube_rtt_scale * offs * offs * offs) >> (10 + 3 * BICTCP_HZ);
    LOG("ca.origin_point: %d, delta: %d \n", ca.origin_point, delta);
    std::cout << "cc_update:ca.origin_point: " << ca.origin_point << std::endl;
    std::cout << "cc_update:delta: " << delta << std::endl;

    if (t < ca.K) /* below origin*/
        target = ca.origin_point - delta;
    else
        /* above origin*/
        target = ca.origin_point + delta;
    LOG("target: %d \n", target);
    std::cout << "cc_update:target: " << target << std::endl;

    if (target > cwnd)
        ca.cnt = cwnd / (target - cwnd);
    else
        ca.cnt = 100 * cwnd;

    if(ca.loss_cwnd == 0)
        ca.cnt=50;

    //Below code is not present in NS2 but in kernel code
    /*if (ca.last_max_cwnd == 0 && ca.cnt > 20)
        ca.cnt = 20;*/
    LOG("ca.cnt: %d \n",ca.cnt);
    std::cout << "ca.cnt1: " << ca.cnt << std::endl;

    if (tcp_friendliness) {
        //cc_friendliness(cwnd);
        LOG("Inside TCP FRIENDLINESS \n");
        appInt32 scale = beta_scale;

         delta = (cwnd * scale) >> 3;
         while (ca.ack_cnt > delta) {
         ca.ack_cnt = ca.ack_cnt - delta;
         ca.tcp_cwnd++;
         }

         if(ca.tcp_cwnd > cwnd){
             delta= ca.tcp_cwnd - cwnd;
             LOG("DELTA: %d \n", delta);
             max_cnt = cwnd/ delta;
             if(ca.cnt > max_cnt)
                 ca.cnt = max_cnt;
         }
         LOG("ca.delayed_ack: %d \n", ca.delayed_ack);
         if(ca.delayed_ack)    // If statement is not present in actual algoritm. it is addded for ca.delayed_ack=0
             ca.cnt = (ca.cnt << ACK_RATIO_SHIFT) / ca.delayed_ack;
         if(ca.cnt == 0)
             ca.cnt = 1;
         LOG("Inside TCP FRIENDLINESS \n");
    }
    LOG("ca.cnt: %d \n",ca.cnt);
    std::cout << "ca.cnt2: " << ca.cnt << std::endl;
}

void CubicChannelHandler::ack_rcv(appInt16 acked) {
    LOG("Inside ack_rcv\n");
    if (ca.delay_min)
        ca.delay_min = min(ca.delay_min, RTT);
    else
        ca.delay_min = RTT;
    if (CWND <= ssthresh) {
        std::cout << "CWND:" << CWND << "\n";
        cube_factor = CWND + acked;
        CWND = CWND + acked;
        LOG("ack_rcv:CWND: %d \n", CWND);
        std::cout << "CWND:" << CWND << "\n";
    } else {
        cc_update(CWND, acked);
        if (snd_cwnd_cnt > ca.cnt) {
            CWND = CWND + 1;
            snd_cwnd_cnt = 0;
        } else {
            snd_cwnd_cnt = snd_cwnd_cnt + acked;
            if (snd_cwnd_cnt >= ca.cnt) {
                appInt16 delta = snd_cwnd_cnt / ca.cnt;
                snd_cwnd_cnt -= delta * ca.cnt;
                CWND += delta;
            }
        }

    }
    LOG("Exit ack_rcv\n");
}

void CubicChannelHandler::packet_loss() {
    LOG("Inside pakcet_loss\n");
    ca.epoch_start = 0;
    if (CWND < ca.last_max_cwnd && fast_convergence)
        ca.last_max_cwnd = (CWND * (BICTCP_BETA_SCALE + beta)) / (2 * beta_scale);
    else
        ca.last_max_cwnd = CWND;

    ca.loss_cwnd = CWND;

    //CWND = CWND * (1 - beta); // according to theory
    CWND = max( (CWND * beta) / BICTCP_BETA_SCALE, 2U); // updated
    ssthresh = CWND;
    LOG("SSTHRESH: %d, CWND: %d, CA.LAST_MAX_CWND: %d \n",ssthresh, CWND, ca.last_max_cwnd );
    LOG("Exit pakcet_loss\n");
}

void CubicChannelHandler::firstTimeoutCalculation(appSInt64 RTT) {
    LOG("Inside firstTimeoutCalculation\n");
    SRTT = RTT;
    RTTVAR = RTT / 2;
    //RTO = SRTT + max(G, K * RTTVAR); G = 100 msec
    RTO = SRTT + K * RTTVAR;

    if (RTO < 1000) // If RTO less than 1 sec
        RTO = 1000;
    LOG("RTTVAR: %d, SRTT: %d, RTO: %d \n",RTTVAR, SRTT, RTO);
    LOG("Exit firstTimeoutCalculation\n");

}

void CubicChannelHandler::timeoutCalculation(appSInt64 RTT) {
    LOG("Inside timeoutCalculation\n");
    RTTVAR = (mulBeta * RTTVAR) / scaleBeta + abs(SRTT - RTT) / scaleBeta; //RTTVAR <- (1 - beta) * RTTVAR + beta * |SRTT - R'|
    SRTT = (mulAlpha * SRTT) / scaleAlpha + RTT / scaleAlpha; // SRTT <- (1 - alpha) * SRTT + alpha * R'
    //RTO = SRTT + max(G, K * RTTVAR); G=100 msec
    RTO = SRTT + K * RTTVAR;

    if (RTO < 1000) // If RTO less than 1 sec
        RTO = 1000;
    LOG("RTT: %d \n", RTT);
    LOG("RTTVAR: %d, SRTT: %d, RTO: %d \n",RTTVAR, SRTT, RTO);
    LOG("Exit timeoutCalculation\n");
}
