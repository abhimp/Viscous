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
#include "../../util/AppStack.hpp"

#define EMPTY_FINGER_PRINT 0

#define MAX_PACKET_DATA_SIZE 2048
#define MAX_PACKET_SENT_DATA_SIZE 1300

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
};

struct PacketOptionalAbstractHeader{
    PacketOptionalAbstractHeader(appInt8 len, OptionalHeaderType type):next(NULL), len(len+2), type(type){}
    PacketOptionalAbstractHeader *next;
    appInt8 len; // in byte
    OptionalHeaderType type;
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

class Packet{
public:
	Packet(): APP_LL_INIT_LIST, capa(0), len(0), header(), data(NULL), accepted(FALSE), optHeaders(NULL){}
	void reInitHeader(){APP_LL_RESET; len = 0; header.reinit(); accepted = FALSE; optHeaders = NULL;};
	APP_LL_DEFINE(Packet);
	appInt capa;
	appInt len;
	PacketHeader header;
    appByte *data;
    appBool accepted;
    sockaddr_in src_addr;
    sockaddr_in dest_addr;
    PacketOptionalAbstractHeader *optHeaders;
    static const appInt8 maxOptHeader = 16;

};

appStatus generateFingerPrint(appInt16 *fingerPrint, sockaddr_in &remoteAddr);

Packet *decodeHeader(appByte *data, appInt dataLen);
appInt encodeHeader(Packet *pkt, appByte *data, appInt dataLen);

#endif /* SRC_TUNNELLIB_PACKET_H_ */
