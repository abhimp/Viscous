/*
 * Packet.cpp
 *
 *  Created on: 15-Dec-2016
 *      Author: abhijit
 */

#include "Packet.h"
#include "PacketPool.h"
#include <cstring>
#include <arpa/inet.h>

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

inline PacketAckHeader *decodeOptionalAckHeader(appByte *data, appInt dataLen){
    PacketAckHeader *pktAckHdr = GET_OPTIONAL_PACKET_HEADER_TYPE_ACK_HEADER;
    READ_INT_16(pktAckHdr->flowId, data, dataLen);
    READ_INT_16(pktAckHdr->ackNo, data, dataLen);
    return pktAckHdr;
}

inline PacketOptionalAbstractHeader *decodeOptionalHeader(appByte *data, appInt dataLen){
    appInt8 type, len;
    READ_INT_8(type, data, dataLen);
    READ_INT_8(len, data, dataLen);
    PacketOptionalAbstractHeader *pktHdr = NULL;
    switch(type){
        case OPTIONAL_PACKET_HEADER_TYPE_ACK_HEADER:
            pktHdr = decodeOptionalAckHeader(data, dataLen);
            break;
        case OPTIONAL_PACKET_HEADER_TYPE_IP_ADDR:
            pktHdr = decodeOptionalIpHeader(data, dataLen);
            break;
        default:
            APP_ASSERT("invalid optional header");
    }
    APP_ASSERT(len == pktHdr->len);
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
	    ADJUST_DATA_LEN(tmp->len, data, dataLen);
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

inline void encodeOptionalAckHeader(PacketOptionalAbstractHeader *pktOptHdr, appByte *data, appInt dataLen){
    PacketAckHeader *pktAckHdr = (PacketAckHeader *)pktOptHdr;
    WRITE_INT_16(pktAckHdr->flowId, data, dataLen);
    WRITE_INT_16(pktAckHdr->ackNo, data, dataLen);
}

inline void encodeOptionalHeader(PacketOptionalAbstractHeader *pktOptHdr, appByte *data, appInt dataLen){
    WRITE_INT_8(pktOptHdr->type, data, dataLen);
    WRITE_INT_8(pktOptHdr->len, data, dataLen);
    switch(pktOptHdr->type){
        case OPTIONAL_PACKET_HEADER_TYPE_ACK_HEADER:
            encodeOptionalAckHeader(pktOptHdr, data, dataLen);
            break;
        case OPTIONAL_PACKET_HEADER_TYPE_IP_ADDR:
            encodeOptionalIpHeader(pktOptHdr, data, dataLen);
            break;
        default:
            APP_ASSERT("invalid header type");
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
	tmp8 |= pkt->header.OHLen;
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
	    ADJUST_DATA_LEN(tmpAckHead->len, data, dataLen);
	}

	if(pkt->data && pkt->len){
	    std::memcpy(data, pkt->data, pkt->len);
	    dataLen -= pkt->len;
	}
	return tempLen - dataLen;
}
