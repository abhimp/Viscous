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
 * StopNWait.cc
 *
 *  Created on: 13-Aug-2016
 *      Author: abhijit
 */

#include "StopNWait.hpp"
#include <iostream>

StopNWait::StopNWait(BaseReliableObj *parent, appInt16 flowId):
ReliabilityMod(parent, flowId),
receiveData(1024),
bufferLen(0),
startedRecv(FALSE),
firstPacket(TRUE),
waitingForFinAck(FALSE),
//waitingForFinACK(FALSE),
//waitingForFinFINACK(FALSE),
//conClosed_(TRUE),
expectedSeq(0),
//expectedAck(0),
nextSeqNumber(0),
sndHead(NULL),
sndTail(NULL),
lastSent(0, 0),
totalPktRcvd(0),
totalDataRcvd(0),
totalDataAcked(0)
#ifdef __EXTREAME_DEBUG_COND
,rcvHd(NULL), rcvTl(NULL)
,sndHd(NULL), sndTl(NULL)
#endif
{
	pthread_mutex_init(&sendDataLock, 0);
	pthread_mutex_init(&sendPacketLock, 0);
	sem_init(&sendBufferLock, 0, 0);
	if(timeoutProducer)
		timeoutProducer->attach(this);
//	pthread_mutex_init(&conCloseLock, 0);
}

StopNWait::~StopNWait(){
	if(timeoutProducer)
		timeoutProducer->detach(this);
}

void StopNWait::printStat(){
	std::cout << " nextSeqNumber: " << nextSeqNumber << " sndHead: " << sndHead << " sndTail: " << sndTail << " lastSent: " << lastSent \
			<< " totalPktRcvd: " << totalPktRcvd << " totalDataRcvd: " << totalDataRcvd << " totalDataAcked: " << totalDataAcked << std::endl;
}

appSInt StopNWait::sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen){
	return parent->sendPacketTo(this->id, pkt, dest_addr, addrlen);
}

appInt StopNWait::sendDataOrAckPacket(int pendingAck){
	Packet pkt;
	appBool sendData = FALSE;
	pthread_mutex_lock(&sendPacketLock);
	if(pendingAck){
		pkt.header.ackNum = pendingAck;
		pkt.header.flag |= FLAG_ACK;
		sendData = TRUE;
	}
//	if(!expectedAc)
	if(sndHead){
		pkt.data = sndHead->data;
		pkt.capa = pkt.len = sndHead->dlen;
		pkt.header.seqNum = sndHead->seqNum;
		pkt.header.dummy = pkt.len;
		if(sndHead->fin) {
			pkt.header.flag |= FLAG_FFN;
		}
		if(sndHead->firstPacket){
			pkt.header.flag |= FLAG_DAT;
		}
//		expectedAck = sndHead->seqNum;
		sendData = TRUE;
	}
	if(sendData){
#ifdef __EXTREAME_DEBUG_COND
		std::cout << "SEND: FLOW:id: " << flowId << " len:" << pkt.len << " seqNum: " << pkt.header.seqNum << std::endl;
#endif
		sendPacketTo(id, &pkt, NULL, 0);
		lastSent = getTime();
	}
	pthread_mutex_unlock(&sendPacketLock);
	return 0;
}

appSInt StopNWait::recvPacketFrom(Packet *pkt, struct sockaddr_in *src_addr, socklen_t addrlen){
	appInt16 pendingAck = 0;
	appBool newData = FALSE;
	appBool recvFin = FALSE;
	if(!startedRecv){
		if((pkt->header.flag&FLAG_DAT)){
			expectedSeq = pkt->header.seqNum;
			startedRecv = TRUE;
		}
	}

//	if((pkt->header.flag&FLAG_FFN)){
//		LOGD("Close flow recv");
//		if(!waitingForFinAck)
//			closeFlow(flowId());
//
//		pendingAck = expectedSeq;
//		expectedSeq += 1;
//		if(!expectedSeq) expectedSeq += 1;
//	}

	if(startedRecv && (pkt->len || (pkt->header.flag&FLAG_FFN)) && pkt->header.seqNum == expectedSeq && pkt->len == pkt->header.dummy){
		if(pkt->len){
			receiveData.write(pkt->data, pkt->len);
			totalPktRcvd += 1;
			totalDataRcvd += pkt->len;
			newData = TRUE;
		}
		if((pkt->header.flag&FLAG_FFN)){
			LOGD("Close flow recv");
			if(!waitingForFinAck)
				closeFlow(flowId());
		}

#ifdef __EXTREAME_DEBUG_COND
		debugData *pp = new debugData();
//		pp->data = new appByte[pkt->len];
//		std::memcpy(pp->data, pkt->data, pkt->len);
		pp->len = pkt->len;
		pp->seqNum = pkt->header.seqNum;
		pp->pkt.header = pkt->header;
		if(!rcvHd){
			rcvHd = rcvTl = pp;
		}
		else{
			rcvTl->next = pp;
			rcvTl = pp;
		}
		std::cout << "RCV: FLOW:id: " << flowId << " len:" << pkt->len << " seqNum: " << pkt->header.seqNum << std::endl;
#endif
		pendingAck = expectedSeq;
		expectedSeq += 1;
		if(!expectedSeq) expectedSeq += 1;
	}
	if((pkt->header.flag&FLAG_ACK) && sndHead && sndHead->seqNum == pkt->header.ackNum){
		if(sndHead->fin){
			LOGD("Close flow ack recv");
			recvFin = TRUE;
		}
//		expectedAck = 0;
		SendBuffer *tmp = sndHead;
		pthread_mutex_lock(&sendDataLock);
		totalDataAcked += sndHead->dlen;
		sndHead = sndHead->next;
		if(!sndHead)
			sndTail = sndTail->next;
		bufferLen --;
		pthread_mutex_unlock(&sendDataLock);
		if(bufferLen == 1)
			sem_post(&sendBufferLock);
#ifdef __EXTREAME_DEBUG_COND
		debugData *pp = new debugData();
//		pp->data = new appByte[tmp->dlen];
//		std::memcpy(pp->data, tmp->data, tmp->dlen);
		pp->len = tmp->dlen;
		pp->seqNum = tmp->seqNum;
		if(!sndHd){
			sndHd = sndTl = pp;
		}
		else{
			sndTl->next = pp;
			sndTl = pp;
		}
		std::cout << "SND: FLOW:id: " << flowId << " len:" << tmp->dlen << " seqNum: " << tmp->seqNum << std::endl;
#endif
		delete tmp;
	}
	sendDataOrAckPacket(pendingAck);
//	conClosed() = FALSE;
	if(newData) sendNotification(EVENT_NEW_DATA, flowId(), FALSE);
	if(recvFin) sendNotification(EVENT_FINISHED, flowId(), FALSE);
	return 0;
}

appSInt StopNWait::readData(appInt16 flowId, appByte *data, appInt size){
	return receiveData.read(data, size);
}

appSInt StopNWait::sendData(appInt16 flowId, appByte *data, appInt dataLen){
	appInt len = dataLen;
	appInt pos = 0;
	while(len){
		appInt nextLen = SendBuffer::maxDLen < len ? SendBuffer::maxDLen : len;
		SendBuffer *buf = new SendBuffer(data + pos, nextLen, getNextSeqNumber(), firstPacket);
		if(firstPacket) firstPacket = FALSE;
		pos += nextLen;
		len -= nextLen;
		pthread_mutex_lock(&sendDataLock);
		if(sndHead && sndTail){
			sndTail->next = buf;
			sndTail = buf;
		}
		else{
			sndHead = sndTail = buf;
		}
		bufferLen ++;
		pthread_mutex_unlock(&sendDataLock);
		if(bufferLen == 2)
			sem_wait(&sendBufferLock);
	}
	sendDataOrAckPacket();
	return 0;
}

appInt StopNWait::timeoutEvent(appTs time){
	if(sndHead && time > lastSent+0.3)
		sendDataOrAckPacket();
	return 0;
}

appInt16 StopNWait::getNextSeqNumber(){
	nextSeqNumber++;
	if(!nextSeqNumber) nextSeqNumber++; //to avoid 0
	return nextSeqNumber;
}

appStatus StopNWait::closeFlow(appInt16 flowId){
	SendBuffer *buf = new SendBuffer(NULL, 0, getNextSeqNumber(), FALSE);
	buf->fin = TRUE;
	pthread_mutex_lock(&sendDataLock);
	if(sndHead && sndTail){
		sndTail->next = buf;
		sndTail = buf;
	}
	else{
		sndHead = sndTail = buf;
	}
	waitingForFinAck = TRUE;
	pthread_mutex_unlock(&sendDataLock);
	return APP_SUCCESS;
}
