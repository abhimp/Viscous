/*
 * StreamHandler.hpp
 *
 *  Created on: 12-Dec-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_ARQ_STREAMHANDLER_HPP_
#define SRC_TUNNELLIB_ARQ_STREAMHANDLER_HPP_
#include "../CommonHeaders.hpp"
#include "../../util/CircularBuffer.hpp"
#include "../../util/BitField.hpp"
#include "../../util/ConditonalWait.hpp"
#include <queue>
#include "../../util/ThreadPool.h"
#include "../../util/ToZeroCounter.h"

class StreamHandler: public ReliabilityMod {
public:
    StreamHandler(BaseReliableObj *parent, appInt16 flowId);
    virtual ~StreamHandler();

	virtual appSInt sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen);
	virtual appSInt recvPacketFrom(Packet *pkt);
	virtual void initiateClosure();
//	virtual appSInt readData(appInt16 flowId, appByte *data, appInt size);
	virtual appSInt sendData(appInt16 flowId, appByte *data, appInt dataLen);
	virtual appStatus recvAck(PacketAckHeader *pktAckHdr);
	virtual appInt timeoutEvent(appTs time);
	virtual appStatus closeFlow(appInt16 flowId);
	virtual appSInt recvAck(appInt16 flowId, appInt16 flowFeqNo);
    virtual void setCallBack(EventType evt, void *info, void *func);
	void printStat();

private:
	appInt16 getNextSeqNo();
	void sendClosePacket();
	void sendAck(appInt16 ackNo);
	static void *sendRecvdPacket(void *data);
//	void prepareAck();
	void finishClosing();
	static void* sendClosePacketInsideThread(void *data);
    void runInThreadPool(UTIL::cb_func fn, void* data);

	CircularBuffer<appByte> recvBuffer, sendBuffer;
	BitField ackedBitField;
	appInt16 nextSeqNumber;
	Packet **recvQueue;
	appBool firstPacketReceived;
	appInt16 recvdPktUpto;

	appInt16 readPacketUpto, ackedPacketUpto; //LIMITED_MEMORY

	appInt16 recvAckedPacketOffset;
//	BitField ackedBuffer;
	ConditionalWait waitForSenderSlot;
	AppSemaphore waitForCloseReport;
	ConditionalWait waitToRead;
	appBool connectionClosed, closingInitiated;
	appSInt packetsPendingToRead;
	appSInt64 totalPacketsRecvd;
	appInt16 jobsInThread;
	newDataToFlow newDataCB;
	closingFlow closingFlowCB;
	void *newDataCBInfo, *closingFlowCBInfo;
	std::mutex limitedMemoryLock;
	std::mutex readPacketLock;
	std::mutex jobInThreadLock;
	std::mutex closingMutex;
	ConditionalWait waitToDestroy;
	UTIL::WorkerThread *worker;
	ToZeroCounter<int> threadJobCounter;
};

#endif /* SRC_TUNNELLIB_ARQ_STREAMHANDLER_HPP_ */
