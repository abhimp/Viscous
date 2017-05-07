/*
 * ServerConnectionClientDetail.cc
 *
 *  Created on: 14-Aug-2016
 *      Author: abhijit
 */
#include "ServerConnectionClientDetail.hpp"
#include "ServerConnection.hpp"
#include "PacketPool.h"

//===================
//  ClientDetails
//===================
ClientDetails::ClientDetails(appInt16 fingerPrint, Multiplexer *mux, ServerConnection *parent):
			BaseReliableObj(parent), clientFingerPrint(fingerPrint), remoteAddr(), mux(mux),
			lastUsed(), parent(parent), ifcSch(this, TRUE)
{
}

void ClientDetails::setupInterfaces(appInt8 ifcRem, sockaddr_in &remAddr){
    RemoteAddr rm(remAddr.sin_addr, remAddr.sin_port);
    ifcSch.addRemote(ifcRem, &rm);
}

appSInt ClientDetails::sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen){
	APP_ASSERT(pkt);
	pkt->header.fingerPrint = clientFingerPrint;
	appSInt ret = ifcSch.sendPacketTo(id, pkt, dest_addr, addrlen);
	return ret;
}

appSInt ClientDetails::recvPacketFrom(Packet *pkt){
	APP_ASSERT(pkt);
	appSInt ret = 0;
	if(pkt->header.fingerPrint != clientFingerPrint){
		ret = 1;
		goto out;
	}
	if(!this->mux){
		ret = 2;
		goto out;
	}
	ifcSch.recvPacketFrom(pkt);
    ret = mux->recvPacketFrom(pkt);
    out:
    if(ret){
        getPacketPool().freePacket(pkt);
    }

    return 0;
}

ARQ::Streamer *ClientDetails::addNewFlow(Packet *pkt, sockaddr_in &src_addr, sockaddr_in &dest_addr){
	if(pkt->header.fingerPrint != clientFingerPrint)
		return NULL;
	if(!this->mux)
		return NULL;

	return mux->addNewFlow(pkt, src_addr, dest_addr);
}


appSInt ClientDetails::readData(appInt16 flowId, appByte *data, appInt size){
	return mux->readData(flowId, data, size);
}

appSInt ClientDetails::sendData(appInt16 flowId, appByte *data, appInt size){
	return mux->sendData(flowId, data, size);
}

appInt ClientDetails::timeoutEvent(appTs time){
	if(mux)
		mux->timeoutEvent(time);
	return 0;
}

void ClientDetails::getOption(APP_TYPE::APP_GET_OPTION optType,
        void* optionValue, appInt optionValueLen, void* returnValue,
        appInt returnValueLen) {
    switch(optType){
        case APP_TYPE::APP_GET_STREAM_FLOW:
            mux->getOption(optType, optionValue, optionValueLen, returnValue, returnValueLen);
            break;
        default:
            APP_ASSERT(0);
    }
}

void ClientDetails::close() {
    mux->close();
    LOGI("ifcSch closing");
    ifcSch.close();
}
