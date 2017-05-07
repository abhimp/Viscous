/*
 * InterfaceHnadler.hpp
 *
 *  Created on: 06-Dec-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_CHANNELHANDLER_HPP_
#define SRC_TUNNELLIB_CHANNELHANDLER_HPP_
#include <common.h>
#include "CommonHeaders.hpp"
#include "Connection.hpp"
#include "InterfaceController/SearchMac.hpp"
#include "InterfaceController/SendThroughInterface.h"

class ChannelHandler : public TimeoutObserver, public PacketFlags, public AppTimeCla{
public:
//    ChannelHandler(BaseReliableObj *parent, InterfaceAddr &intfc, RemoteAddr &rmadr);
//    ChannelHandler(BaseReliableObj *parent, InterfaceAddr &intfc, RemoteAddr &rmadr, appInt localPort);
    ChannelHandler(BaseReliableObj *parent, appInt8 ifcLoc, RemoteAddr &rmadr, appInt8 ifcSrc, appInt8 ifcDst);
	virtual ~ChannelHandler();
	virtual appInt16 getSenderWindowSize();
	virtual appSInt sendPacket(Packet *pkt);
	virtual appSInt recv(Packet *pkt);
	virtual bool haveCell();
	virtual appInt &id();
	virtual appInt timeoutEvent(appTs time);
//	virtual appTs getTime(){ appTime x; appGetSysTime(&x); appTs y(0,0); y.nsec() = x.tv_nsec; y.sec() = x.tv_sec; return y;}
//	virtual void setIfcId(appInt8 ifcSrc, appInt8 ifcDst);
	virtual void waitToMoreSpace() = 0;
	virtual void freePacket(Packet *pkt);
	virtual void sendFreeCellNotification();
	virtual void shutDown();
protected:
	virtual appSInt send(Packet *pkt);
	BaseReliableObj *parent;
private:
	SendThroughInterface *sender;
	RemoteAddr remoteAddr;
	appInt32 remIp;
	appInt8 ifcSrc, ifcDst;
	appInt id_;

};

#endif /* SRC_TUNNELLIB_CHANNELHANDLER_HPP_ */
