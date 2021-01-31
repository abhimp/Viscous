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
 * Packet.h
 *
 *  Created on: 15-Dec-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_PACKET_H_
#define SRC_TUNNELLIB_PACKET_H_

#include <common.h>
#include <arpa/inet.h>
#include <Profiler.hh>

#include "../util/AppLLQueue.hh"
#include "../util/AppStack.hh"

#define EMPTY_FINGER_PRINT 0

#define MAX_PACKET_DATA_SIZE 2048
#define MAX_PACKET_SENT_DATA_SIZE 1300

#ifdef __PROFILER_ENABLED__
#define PACKET_PROF(x) (x->prof)

#define PROFILE_PERPACKET_WAIT_START(pkt) PROFILE_PERPACKET_WAIT_START_DUMMY(PACKET_PROF(pkt), __FILE__, __LINE__);
#define PROFILE_PERPACKET_WAIT_STOP(pkt) PROFILE_PERPACKET_WAIT_STOP_DUMMY(PACKET_PROF(pkt), Profiler::getTime(), __FILE__, __LINE__);
#define PROFILE_PERPACKET_WAIT_STOP_ALL(pkt) {auto t = Profiler::getTime();for(Packet *x = (pkt); x; x=(Packet *)x->next){PROFILE_PERPACKET_WAIT_STOP_DUMMY(PACKET_PROF(pkt), t, __FILE__, __LINE__);}}

#define START_PACKET_PROFILER_CLOCK(pkt) {(pkt)->ticks = Profiler::getTime();}
#define START_PACKET_PROFILER_CLOCK_ALL(pkt) {for(Packet *x = (pkt); x; x=(Packet *)x->next){ x->ticks = Profiler::getTime();}}
#define STOP_PACKET_PROFILER_CLOCK(pkt) {if((pkt)) {PROFILE_PACKET_WAIT_UPDATE((pkt)->ticks, PACKET_PROF(pkt));}}
#define STOP_PACKET_PROFILER_CLOCK_ALL(pkt) {for(Packet *x = (pkt); x; x=(Packet *)x->next){ PROFILE_PACKET_WAIT_UPDATE(x->ticks, PACKET_PROF(pkt));}}
#else
#define PROFILE_PERPACKET_WAIT_START(pkt)
#define PROFILE_PERPACKET_WAIT_STOP(pkt)
#define PROFILE_PERPACKET_WAIT_STOP_ALL(pkt)

#define START_PACKET_PROFILER_CLOCK(pkt)
#define START_PACKET_PROFILER_CLOCK_ALL(pkt)
#define STOP_PACKET_PROFILER_CLOCK(pkt)
#define STOP_PACKET_PROFILER_CLOCK_ALL(pkt)
#endif

/*
#======================================================================
#
#                          Custom Header Format
#
#   0               1               2               3
#   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
#  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#  |  Ver  | OHLen | ifc s | ifc r |             Flag              |
#  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#  |        Sequence Number        |           Ack Number          |
#  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#  |          Window Size          |        Finger Print           |
#  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#  |            Flow Id            |     Flow Sequence Number      |
#  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#  |      Original Ack Number      |            Padding            |
#  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#  |                                                               |
#  +-+-+-+-+-+-+-+-+-+-+-+  Sent Time Stamp  +-+-+-+-+-+-+-+-+-+-+-+
#  |                                                               |
#  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#  |                     Optional Ack Header                       |
#  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#  |                             data                              |
#  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#
#======================================================================
*/
#define PACKET_HEADER_SIZE 28

/*
#======================================================================
#
#                        Custom Acked Header Format
#
#   0               1               2               3
#   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
#  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#  |           Flow Id             |            Ack No             |
#  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#
#======================================================================
*/

enum OptionalHeaderType{
    OPTIONAL_PACKET_HEADER_TYPE_INVALID = 0,
    OPTIONAL_PACKET_HEADER_TYPE_ACK_HEADER,
    OPTIONAL_PACKET_HEADER_TYPE_IP_ADDR,
    OPTIONAL_PACKET_HEADER_TYPE_NONCE_HEADER,
    OPTIONAL_PACKET_HEADER_TYPE_IP_CHANGE,
    OPTIONAL_PACKET_HEADER_TYPE_READ_HEADER,
};

struct PacketOptionalAbstractHeader{
    PacketOptionalAbstractHeader(appInt16 len, OptionalHeaderType type):next(NULL), len_(len+4), type(type){}
    virtual ~PacketOptionalAbstractHeader(){};
    PacketOptionalAbstractHeader *next;
    appInt16 len_; // in byte
    appInt8 pad_;
    OptionalHeaderType type;
    virtual appInt16 len(){return len_;}
};

struct ReceiverInfo{
    ReceiverInfo():next(NULL), flowId(0), readUpto(0){}
    ReceiverInfo *next;
    appInt16 flowId;
    appInt16 readUpto;
};

struct PacketReadHeader : public PacketOptionalAbstractHeader{
    PacketReadHeader(): PacketOptionalAbstractHeader(2, OPTIONAL_PACKET_HEADER_TYPE_READ_HEADER), readerInfo(NULL), count(0){}
    ~PacketReadHeader();
    ReceiverInfo *readerInfo;
    appInt16 count;
    void addInfo(appInt16 flowId, appInt16 readUpto);
    appInt16 decode(appByte *data, appInt dataLen);
    appInt16 encode(appByte *data, appInt dataLen);
    virtual appInt16 len();
};

struct PacketAckHeader : public PacketOptionalAbstractHeader
{
    PacketAckHeader():PacketOptionalAbstractHeader(4, OPTIONAL_PACKET_HEADER_TYPE_ACK_HEADER),flowId(0), ackNo(0){}
    appInt16 flowId;
    appInt16 ackNo;
};

struct PacketIpHeader : public PacketOptionalAbstractHeader //
{
    PacketIpHeader(): PacketOptionalAbstractHeader(6, OPTIONAL_PACKET_HEADER_TYPE_IP_ADDR), ifcId(0), ifcType(0), ip(0){}
    appInt8 ifcId; //value wont be more than 15
    appInt8 ifcType; //wlan or ether net
    appInt32 ip;
};

struct PacketIpChngHeader : public PacketOptionalAbstractHeader //
{
//    enum IPEvent { IP_REMOVED, IP_ADDED };
    PacketIpChngHeader(): PacketOptionalAbstractHeader(2, OPTIONAL_PACKET_HEADER_TYPE_IP_CHANGE), ifcId(0), ifcInfo(0){}
    appInt8 ifcId; //value wont be more than 15
    union{
        appInt8 ifcInfo;
        struct{
        appInt8 ifcType:2, //wlan or ether net
                removed:1, // removed(1);
                added:1;
        };
    };
};

struct PacketNonceHeader : public PacketOptionalAbstractHeader
{
    PacketNonceHeader() : PacketOptionalAbstractHeader(8, OPTIONAL_PACKET_HEADER_TYPE_NONCE_HEADER), nonce(0){}
    appInt64 nonce;
};

struct PacketHeader{
    PacketHeader():
        version(0), OHLen(0), ifcsrc(0), ifcdst(0), flag(0),
        seqNo(0), ackNo(0),
        window(0), fingerPrint(0),
        flowId(0), flowSeqNo(0),
        origAckNo(0), padding(0),
        sentTS(0){}
    void reinit(){
        version = 0;
        OHLen = 0;
        ifcsrc = 0;
        ifcdst = 0;
        flag = 0;

        seqNo = 0;
        ackNo = 0;

        window = 0;
        fingerPrint = 0;

        flowId = 0;
        flowSeqNo = 0;

        origAckNo = 0;
        padding = 0;

        sentTS = 0;
    }
    appInt8 version;
    appInt8 OHLen;
    appInt8 ifcsrc;
    appInt8 ifcdst;
    appInt16 flag;

    appInt16 seqNo;
    appInt16 ackNo;

    appInt16 window;
    appInt16 fingerPrint;

    appInt16 flowId;
    appInt16 flowSeqNo;

    appInt16 origAckNo;
    appInt16 padding;

    appSInt64 sentTS; //to be used in calculation of rtt

};

class Packet:public util::LL_Node{
public:
    Packet(): util::LL_Node(), capa(0), len(0), header(), data(NULL), accepted(FALSE), newFlow(FALSE), processed(FALSE), recvTS(0), optHeaders(NULL)
#ifdef __PROFILER_ENABLED__
    ,ticks(0), prof(new Profiler::PerPacketProfile())
#endif
    {}
    void reInitHeader(){len = 0; header.reinit(); accepted = FALSE; newFlow = FALSE; processed = FALSE; recvTS = 0; optHeaders = NULL;
#ifdef __PROFILER_ENABLED__
    ticks = 0;
    if (prof)
        	delete prof;
    auto x = new Profiler::PerPacketProfile();
    prof = x; //std::shared_ptr<Profiler::PerPacketProfile>(x);
#endif
    };
    appInt capa;
    appInt len;
    PacketHeader header;
    appByte *data;
    appByte accepted:1,
            newFlow:1,
            processed:1;
    appSInt64 recvTS;
    sockaddr_in src_addr;
    sockaddr_in dest_addr;
    PacketOptionalAbstractHeader *optHeaders;
#ifdef __PROFILER_ENABLED__
    Profiler::ProfTime ticks;
    Profiler::PerPacketProfile *prof;
#endif
    static const appInt8 maxOptHeader = 16;
    void operator=(Packet other);
    void operator=(Packet *other);
    void cloneWithoutData(Packet *pkt);
    void cloneWithData(Packet *pkt);
};

appStatus generateFingerPrint(appInt16 *fingerPrint, sockaddr_in &remoteAddr);

Packet *decodeHeader(appByte *data, appInt dataLen);
appInt encodeHeader(Packet *pkt, appByte *data, appInt dataLen);

#endif /* SRC_TUNNELLIB_PACKET_H_ */
