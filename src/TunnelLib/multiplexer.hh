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
 * multiplexer.h
 *
 *  Created on: 04-Aug-2016
 *      Author: abhijit
 */

#ifndef TUNNELLIB_MULTIPLEXER_HPP_
#define TUNNELLIB_MULTIPLEXER_HPP_
#include <map>
#include "ARQ/Streamer.h"

#include "CommonHeaders.hh"

typedef appInt16 MuxHdr;

class Multiplexer : public BaseReliableObj{
public:
	Multiplexer(BaseReliableObj *base):BaseReliableObj(base), children(), id2FlowMap(){};
	virtual ~Multiplexer();
	virtual appSInt sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen);
	virtual appSInt recvPacketFrom(Packet *pkt);
	virtual appInt timeoutEvent(appTs time);
	virtual ARQ::Streamer *getNewConnection(appInt16 flowId, CongType type = STREAM_HANDLER);
	inline appBool isValidFlow(appInt16 flowId){return hasKey(children, flowId);}
	virtual ARQ::Streamer *addNewFlow(Packet *pkt, sockaddr_in &src_addr, sockaddr_in &dest_addr);
	virtual appSInt readData(appInt16 flowId, appByte *data, appInt size);
	virtual appSInt sendData(appInt16 flowId, appByte *data, appInt dataLen);
	virtual inline appBool flowExists(appInt16 flowId);
	int printStat();

	virtual appStatus closeFlow(appInt16 flowId);
	virtual appStatus close();
	void getOption(APP_TYPE::APP_GET_OPTION optType, void *optionValue, appInt optionValueLen, void *returnValue = NULL, appInt returnValueLen = 0);
	virtual appSInt recvAck(appInt16 flowId, appInt16 flowFeqNo);

private:
	ReliabilityMod *getChild(appInt16 flowId);
	inline void processOptionalHeader(PacketOptionalAbstractHeader *pktOptHdr);
	MuxHdr readHeader(appByte *data, appInt dataLen);
	std::map<MuxHdr, ARQ::Streamer *> children;
	std::map<appInt, MuxHdr> id2FlowMap;
	std::set<appInt16> closedFlows;
	std::mutex accessChildrenMap;
};



#define MUX_HEADER_LEN 4

#endif /* TUNNELLIB_MULTIPLEXER_HPP_ */
