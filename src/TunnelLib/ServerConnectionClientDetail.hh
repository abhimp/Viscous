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
 * ServerConnectionClientDetail.hh
 *
 *  Created on: 14-Aug-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_SERVERCONNECTIONCLIENTDETAIL_HPP_
#define SRC_TUNNELLIB_SERVERCONNECTIONCLIENTDETAIL_HPP_

#include "Connection.hh"
#include "multiplexer.hh"
#include "InterfaceScheduler.h"
#include <appThread.h>
#include <set>
#include <map>

class ServerConnection;

class ClientDetails:public BaseReliableObj{
public:
	ClientDetails(appInt16 fingerPrint, Multiplexer *mux, ServerConnection *parent);
	virtual appInt16 getFingerPrint(){return clientFingerPrint;}
	virtual appSInt sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen);
	virtual appSInt recvPacketFrom(Packet *pkt);
	virtual appInt timeoutEvent(appTs time);
	virtual void setMux(Multiplexer *mux){this->mux = mux;};
	inline appBool isValidFlow(appInt16 flowId){return mux and mux->isValidFlow(flowId);}
	virtual ARQ::Streamer *addNewFlow(Packet *pkt, sockaddr_in &src_addr, sockaddr_in &dest_addr);
	virtual appSInt readData(appInt16 flowId, appByte *data, appInt size);
	virtual appSInt sendData(appInt16 flowId, appByte *data, appInt size);
	virtual appStatus closeFlow(appInt16 flowId){return mux->closeFlow(flowId);};
	virtual void setupInterfaces(appInt8 ifcRem, sockaddr_in &remAddr);
	virtual inline PendingAcks &pendingAcks(void) {return pendAck;}
	virtual void getOption(APP_TYPE::APP_GET_OPTION optType, void *optionValue, appInt optionValueLen, void *returnValue = NULL, appInt returnValueLen = 0);
	virtual void close();
	inline virtual appSInt recvAck(appInt16 flowId, appInt16 flowFeqNo) {assert(mux); return mux->recvAck(flowId, flowFeqNo);};
private:
	appInt16 clientFingerPrint;
	std::set<RemoteAddr> remoteAddr;
	Multiplexer *mux;
	appTs lastUsed;
	ServerConnection *parent;
	InterfaceScheduler ifcSch;
	PendingAcks pendAck;
};

#endif /* SRC_TUNNELLIB_SERVERCONNECTIONCLIENTDETAIL_HPP_ */
