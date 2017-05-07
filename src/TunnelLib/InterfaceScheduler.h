/*
 * InterfaceScheduler.h
 *
 *  Created on: 07-Dec-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_INTERFACESCHEDULER_H_
#define SRC_TUNNELLIB_INTERFACESCHEDULER_H_
#include <set>
#include "CommonHeaders.hpp"
#include "../../util/CircularBuffer.hpp"
#include "../../util/ConditonalWait.hpp"
#include "ChannelHandler.hpp"

#define BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT 16
#define BASIC_CHANNEL_HANDLER_CHANNEL_COUNT 256

class InterfaceScheduler : public BaseReliableObj{
public:
	InterfaceScheduler(BaseReliableObj *parent, appBool doNotAddChannel = FALSE);
	virtual ~InterfaceScheduler();
	appSInt sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen);
	appSInt recvPacketFrom(Packet *pkt);
	appInt timeoutEvent(appTs time);
	appStatus closeFlow(appInt16 flowId);
	void addRemote(appInt8 id, RemoteAddr *remoteAddr);
	void removeRemote(appInt8 id);
	void notifyFreeCell(appInt8 chId);
	void close();
//    void getOption(APP_TYPE::APP_GET_OPTION optType, void *optionValue, appInt optionValueLen, void *returnValue = NULL, appInt returnValueLen = 0){APP_ASSERT(0 && "Not allowed");};
private:
	inline void addChannel(appInt8 ifcLoc, appInt8 ifcRem);
	inline appSInt schedulePacket(Packet *pkt);
	std::vector<ChannelHandler *> interfaces;
	appInt nextInterfaceId;
	appInt next;
	CircularBuffer<Packet *> packetBuffer;
	RemoteAddr *remoteAddresses[BASIC_CHANNEL_HANDLER_REMOTE_ADDRESS_COUNT];
	ChannelHandler *channelHandlers[BASIC_CHANNEL_HANDLER_CHANNEL_COUNT]; // ease of access
	ConditionalWait waitForFreeCell;
	appInt8 freeCellChiID;
	appBool doNotAddChannel;
	std::mutex sendPacketMutex;
	APP_LL_QUEUE_DEFINE(Packet);
	APP_LL_QUEUE_ADD_FUNC(Packet);
	APP_LL_QUEUE_REMOVE_FUNC(Packet);
};

#endif /* SRC_TUNNELLIB_INTERFACESCHEDULER_H_ */
