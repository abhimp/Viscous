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
 * Packet.cpp
 *
 *  Created on: 15-Dec-2016
 *      Author: abhijit
 */

#include "Packet.h"
#include <cstring>
#include <arpa/inet.h>
#include "PacketPool.hh"

#if defined(__linux__)
#  include <endian.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#endif

#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

#define ADJUST_DATA_LEN(_l, _data, _len) \
    _data += _l; \
    _len -= _l;

#define READ_INT_8(_var, _data, _len) \
    _var = *((appInt8 *)_data); \
    ADJUST_DATA_LEN(1, _data, _len)

#define READ_INT_16(_var, _data, _len) \
    _var = ntohs(*((appInt16 *)_data)); \
    ADJUST_DATA_LEN(2, _data, _len)

#define READ_INT_32(_var, _data, _len) \
    _var = ntohl(*((appInt32 *)_data)); \
    ADJUST_DATA_LEN(4, _data, _len)

#define READ_INT_64(_var, _data, _len) \
    _var = be64toh(*((appInt64 *)_data)); \
    ADJUST_DATA_LEN(8, _data, _len)

#define WRITE_INT_8(_var, _data, _len) \
    *((appInt8 *)_data) = _var; \
    ADJUST_DATA_LEN(1, _data, _len)

#define WRITE_INT_16(_var, _data, _len) \
    *((appInt16 *)_data) = htons(_var); \
    ADJUST_DATA_LEN(2, _data, _len)

#define WRITE_INT_32(_var, _data, _len) \
    *((appInt32 *)_data) = htonl(_var); \
    ADJUST_DATA_LEN(4, _data, _len)

#define WRITE_INT_64(_var, _data, _len) \
    *((appInt64 *)_data) = htobe64(_var); \
    ADJUST_DATA_LEN(8, _data, _len)

appStatus generateFingerPrint(appInt16 *fingerPrint, sockaddr_in &remoteAddr){
    static appInt32 id = 1;
    *fingerPrint = id;
    id++;
    APP_RETURN_SUCCESS;
}

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


inline PacketIpHeader *decodeOptionalIpHeader(appByte *data, appInt dataLen){
    PacketIpHeader *pktIpHdr = GET_OPTIONAL_PACKET_HEADER_TYPE_IP_ADDR;
    READ_INT_8(pktIpHdr->ifcId, data, dataLen);
    READ_INT_8(pktIpHdr->ifcType, data, dataLen);
    READ_INT_32(pktIpHdr->ip, data, dataLen);
    return pktIpHdr;
}

inline PacketReadHeader *decodeOptionalReadHeader(appByte *data, appInt dataLen){
    PacketReadHeader *pktReadHdr = GET_OPTIONAL_PACKET_HEADER_TYPE_READ_HDR;
    auto len = pktReadHdr->decode(data, dataLen);
    ADJUST_DATA_LEN(len, data, dataLen);
    return pktReadHdr;
}

inline PacketIpChngHeader *decodeOptionalIpChngHeader(appByte *data, appInt dataLen){
    PacketIpChngHeader *pktIpHdr = GET_OPTIONAL_PACKET_HEADER_TYPE_IP_CHNG_ADDR;
    READ_INT_8(pktIpHdr->ifcId, data, dataLen);
    READ_INT_8(pktIpHdr->ifcInfo, data, dataLen);
    return pktIpHdr;
}


inline PacketAckHeader *decodeOptionalAckHeader(appByte *data, appInt dataLen){
    PacketAckHeader *pktAckHdr = GET_OPTIONAL_PACKET_HEADER_TYPE_ACK_HEADER;
    READ_INT_16(pktAckHdr->flowId, data, dataLen);
    READ_INT_16(pktAckHdr->ackNo, data, dataLen);
    return pktAckHdr;
}

inline PacketNonceHeader *decodeOptionalNonceHeader(appByte *data, appInt dataLen){
    auto pktNonceHdr = GET_OPTIONAL_PACKET_HEADER_TYPE_NONCE;
    READ_INT_64(pktNonceHdr->nonce, data, dataLen);
    return pktNonceHdr;
}

inline PacketOptionalAbstractHeader *decodeOptionalHeader(appByte *data, appInt dataLen){
    appInt8 type, pad;
    appInt16 len;
    READ_INT_8(type, data, dataLen);
    READ_INT_8(pad, data, dataLen);
    READ_INT_16(len, data, dataLen);
    PacketOptionalAbstractHeader *pktHdr = NULL;
    switch(type){
        case OPTIONAL_PACKET_HEADER_TYPE_ACK_HEADER:
            pktHdr = decodeOptionalAckHeader(data, dataLen);
            break;
        case OPTIONAL_PACKET_HEADER_TYPE_IP_ADDR:
            pktHdr = decodeOptionalIpHeader(data, dataLen);
            break;
        case OPTIONAL_PACKET_HEADER_TYPE_NONCE_HEADER:
            pktHdr = decodeOptionalNonceHeader(data, dataLen);
            break;
        case OPTIONAL_PACKET_HEADER_TYPE_IP_CHANGE:
            pktHdr = decodeOptionalIpChngHeader(data, dataLen);
            break;
        case OPTIONAL_PACKET_HEADER_TYPE_READ_HEADER:
            pktHdr = decodeOptionalReadHeader(data, dataLen);
            break;
        default:
            APP_ASSERT(0 and "invalid optional header");
    }
    APP_ASSERT(len == pktHdr->len());
    return pktHdr;
}

Packet *decodeHeader(appByte *data, appInt dataLen){
    if(dataLen < PACKET_HEADER_SIZE)
        return NULL;
    appInt8 tmp8;
    Packet *pkt = getPacketPool().getNewPacketWithData();
    READ_INT_8(tmp8, data, dataLen);
    pkt->header.OHLen = tmp8&0x0F;
    pkt->header.version = tmp8 >> 4;
    READ_INT_8(tmp8, data, dataLen);
    pkt->header.ifcsrc = (tmp8 & 0xF0) >> 4;
    pkt->header.ifcdst = tmp8 & 0x0F;
    READ_INT_16(pkt->header.flag, data, dataLen);

    READ_INT_16(pkt->header.seqNo, data, dataLen);
    READ_INT_16(pkt->header.ackNo, data, dataLen);

    READ_INT_16(pkt->header.window, data, dataLen);
    READ_INT_16(pkt->header.fingerPrint, data, dataLen);

    READ_INT_16(pkt->header.flowId, data, dataLen);
    READ_INT_16(pkt->header.flowSeqNo, data, dataLen);

    READ_INT_16(pkt->header.origAckNo, data, dataLen);
    READ_INT_16(pkt->header.padding, data, dataLen);

    READ_INT_64(pkt->header.sentTS, data, dataLen);

    PacketOptionalAbstractHeader *tmpAckHead = NULL;
    for(appInt8 ohlen = pkt->header.OHLen; ohlen; ohlen--){
        PacketOptionalAbstractHeader *tmp = decodeOptionalHeader(data, dataLen);
        ADJUST_DATA_LEN(tmp->len(), data, dataLen);
        tmp->next = tmpAckHead;
        tmpAckHead = tmp;
    }
    pkt->optHeaders = tmpAckHead;
    pkt->len = dataLen;
    std::memcpy(pkt->data, data, dataLen);
    return pkt;
}

inline void encodeOptionalIpHeader(PacketOptionalAbstractHeader *pktOptHdr, appByte *data, appInt dataLen){
    PacketIpHeader *pktIpHdr = (PacketIpHeader *)pktOptHdr;
    WRITE_INT_8(pktIpHdr->ifcId, data, dataLen);
    WRITE_INT_8(pktIpHdr->ifcType, data, dataLen);
    WRITE_INT_32(pktIpHdr->ip, data, dataLen);
}

inline void encodeOptionalReadHeader(PacketOptionalAbstractHeader *pktOptHdr, appByte *data, appInt dataLen){
    PacketReadHeader *pktReadHdr = (PacketReadHeader *)pktOptHdr;
    auto len = pktReadHdr->encode(data, dataLen);
    ADJUST_DATA_LEN(len, data, dataLen);
}

inline void encodeOptionalIpChngHeader(PacketOptionalAbstractHeader *pktOptHdr, appByte *data, appInt dataLen){
    auto pktIpHdr = (PacketIpChngHeader *)pktOptHdr;
    WRITE_INT_8(pktIpHdr->ifcId, data, dataLen);
    WRITE_INT_8(pktIpHdr->ifcInfo, data, dataLen);
}

inline void encodeOptionalAckHeader(PacketOptionalAbstractHeader *pktOptHdr, appByte *data, appInt dataLen){
    PacketAckHeader *pktAckHdr = (PacketAckHeader *)pktOptHdr;
    WRITE_INT_16(pktAckHdr->flowId, data, dataLen);
    WRITE_INT_16(pktAckHdr->ackNo, data, dataLen);
}

inline void encodeOptionalNonceHeadde(PacketOptionalAbstractHeader *pktOptHdr, appByte *data, appInt dataLen){
    auto *pktNonceHdr = (PacketNonceHeader *)pktOptHdr;
    WRITE_INT_64(pktNonceHdr->nonce, data, dataLen);
}

inline void encodeOptionalHeader(PacketOptionalAbstractHeader *pktOptHdr, appByte *data, appInt dataLen){
    WRITE_INT_8(pktOptHdr->type, data, dataLen);
    WRITE_INT_8(pktOptHdr->pad_, data, dataLen); //although it is useless
    WRITE_INT_16(pktOptHdr->len(), data, dataLen);
    switch(pktOptHdr->type){
        case OPTIONAL_PACKET_HEADER_TYPE_ACK_HEADER:
            encodeOptionalAckHeader(pktOptHdr, data, dataLen);
            break;
        case OPTIONAL_PACKET_HEADER_TYPE_IP_ADDR:
            encodeOptionalIpHeader(pktOptHdr, data, dataLen);
            break;
        case OPTIONAL_PACKET_HEADER_TYPE_NONCE_HEADER:
            encodeOptionalNonceHeadde(pktOptHdr, data, dataLen);
            break;
        case OPTIONAL_PACKET_HEADER_TYPE_IP_CHANGE:
            encodeOptionalIpChngHeader(pktOptHdr, data, dataLen);
            break;
        case OPTIONAL_PACKET_HEADER_TYPE_READ_HEADER:
            encodeOptionalReadHeader(pktOptHdr, data, dataLen);
            break;
        default:
            APP_ASSERT(0 and "invalid header type");
    }
}

appInt encodeHeader(Packet *pkt, appByte *data, appInt dataLen){
    if(dataLen < sizeof(PacketHeader))
        return 0;
    appInt tempLen = dataLen;
    appInt8 tmp8;
    appInt8 opHdCnt = 0;
    PacketOptionalAbstractHeader *tmpAckHead;
    for(tmpAckHead = pkt->optHeaders; tmpAckHead; tmpAckHead = tmpAckHead->next, opHdCnt++);

    pkt->header.OHLen = 0x0F & opHdCnt;
    tmp8 = pkt->header.version << 4;
    tmp8 |= (pkt->header.OHLen & 0x0F);
    WRITE_INT_8(tmp8, data, dataLen);
    tmp8 = (pkt->header.ifcsrc & 0x0F) << 4;
    tmp8 |= (pkt->header.ifcdst & 0x0F);
    WRITE_INT_8(tmp8, data, dataLen);
    WRITE_INT_16(pkt->header.flag, data, dataLen);

    WRITE_INT_16(pkt->header.seqNo, data, dataLen);
    WRITE_INT_16(pkt->header.ackNo, data, dataLen);

    WRITE_INT_16(pkt->header.window, data, dataLen);
    WRITE_INT_16(pkt->header.fingerPrint, data, dataLen);

    WRITE_INT_16(pkt->header.flowId, data, dataLen);
    WRITE_INT_16(pkt->header.flowSeqNo, data, dataLen);

    WRITE_INT_16(pkt->header.origAckNo, data, dataLen);
    WRITE_INT_16(pkt->header.padding, data, dataLen);

    WRITE_INT_64(pkt->header.sentTS, data, dataLen);


    for(tmpAckHead = pkt->optHeaders; tmpAckHead; tmpAckHead = tmpAckHead->next)
    {
        encodeOptionalHeader(tmpAckHead, data, dataLen);
        ADJUST_DATA_LEN(tmpAckHead->len(), data, dataLen);
    }

    if(pkt->data && pkt->len){
        std::memcpy(data, pkt->data, pkt->len);
        dataLen -= pkt->len;
    }
    return tempLen - dataLen;
}

void Packet::cloneWithoutData(Packet* pkt) {
//    auto tmp1 = data;
//    auto tmp2 = optHeaders;
    *this = *pkt;
//    data = tmp1;
//    optHeaders = tmp2;
}

void Packet::cloneWithData(Packet* pkt) {
    cloneWithoutData(pkt);
    memcpy(data, pkt->data, pkt->len);
}

void Packet::operator =(Packet other) {
    auto tmp1 = data;
    auto tmp2 = optHeaders;
    memcpy(this, &other, sizeof(Packet));
//    *this = *pkt;
    data = tmp1;
    optHeaders = tmp2;
}

void Packet::operator =(Packet* other) {
    *this = *other;
}

PacketReadHeader::~PacketReadHeader() {
    auto x = readerInfo;
    while(x){
        auto y = x;
        x = x->next;
        delete y;
    }
    readerInfo = NULL;
}

void PacketReadHeader::addInfo(appInt16 flowId, appInt16 readUpto) {
    auto data = new ReceiverInfo();
    data->flowId = flowId;
    data->readUpto = readUpto;
    data->next = readerInfo;
    readerInfo = data;
    count ++;
}

appInt16 PacketReadHeader::decode(appByte* data, appInt dataLen) {
    appInt16 cnt;
    READ_INT_16(cnt, data, dataLen);
    appInt16 flowId, readUp;
    appInt len = 2;
    for(appInt x = 0; x < cnt; x++){
        if(dataLen < 4)
            break;
        READ_INT_16(flowId, data, dataLen);
        READ_INT_16(readUp, data, dataLen);
        addInfo(flowId, readUp);
        len += 4;
    }
    APP_ASSERT(cnt == count);
    return len;
}

appInt16 PacketReadHeader::encode(appByte* data, appInt dataLen) {
    WRITE_INT_16(count, data, dataLen);
//    appInt16 flowId, readUp;
    appInt len = 2;
    appInt16 cnt = 0;
    for(auto x = readerInfo; x; cnt++){
        APP_ASSERT(dataLen >= 4);
        auto y = x;
        x = x->next;
        WRITE_INT_16(y->flowId, data, dataLen);
        WRITE_INT_16(y->readUpto, data, dataLen);
        len += 4;
//        addInfo(flowId, readUp);
    }
    APP_ASSERT(cnt == count)
    return len;
}

appInt16 PacketReadHeader::len() {
    appInt len = PacketOptionalAbstractHeader::len();
    for(auto x = readerInfo; x; x = x->next)
        len += 4;
    return len;
}
