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
 * CubicChannelHandler.hh
 *
 *  Created on: 25-Dec-2016
 *      Author: sourav
 */

#ifndef SRC_TUNNELLIB_CUBICCHANNELHANDLER_HPP_
#define SRC_TUNNELLIB_CUBICCHANNELHANDLER_HPP_

#include "ChannelHandler.hh"
#include "CongestionHandler.hh"

/*For Congestion Controller*/
#define BICTCP_BETA_SCALE	1024
#define BICTCP_HZ 			10
#define BICTCP_B			4

class CubicChannelHandler: public ChannelHandler {
public:
//	CubicChannelHandler(BaseReliableObj *parent, InterfaceAddr &intfc,
//			RemoteAddr &rmadr);
//	CubicChannelHandler(BaseReliableObj *parent, InterfaceAddr &intfc,
//			RemoteAddr &rmadr, appInt localPort);
	CubicChannelHandler(BaseReliableObj *parent, appInt8 ifcLoc, RemoteAddr &rmadr, appInt8 ifcSrc, appInt8 ifcDst);
	virtual ~CubicChannelHandler();
	appInt16 getCongestionWindowSize();
	void setCongestionWindowSize(appInt16 windowSize);
	appInt16 getSenderWindowSize();
	void setSenderWindowSize();
	appInt16 getRcvrWindowSize();
	void setRcvrWindowSize(appInt16 windowSize);
	appSInt sendPacket(Packet *pkt);
	appSInt sendAck(appInt16 ackToSend, appInt16 fingerPrint, appInt16 flowId);
	appSInt recv(Packet *pkt);
	appSInt ackRecv(Packet *pkt);
	appInt8 checkValidAck(int ackno);
	int checkValidPacket(int expectedSeq, int lastRcvdSeq,
			bool indicatorNextByteExpected, bool indicatorLastByteRcvd,
			int curSeq);
	bool isOrdered(int nextByteExpected, int lastByteRcvd);
	void packetRcv(Packet *pkt);
	appInt timeoutEvent(appTs time);
	void cc_init();
	void cc_reset();
	void cc_friendliness(appInt32 cwnd);
	void cc_update(uint32_t cwnd, uint32_t acked);
	void ack_rcv(appInt16 acked);
	void packet_loss();
	void firstTimeoutCalculation(appSInt64 RTT);
	void timeoutCalculation(appSInt64 RTT);
	virtual void waitToMoreSpace() {};

private:
	struct sender_buffer send_buffer;
	struct receiver_buffer rcv_buffer;
	struct Congestion ca;
	appInt16 advertisedWindow; // This is for sender side (Received from ACK packet)
	appInt16 rcvrWindow; // This is for Receiver side.
	appInt16 seqNumberExpected;
	appInt16 seqNumberRcvd;
	appInt16 curr_ackno;
	appInt16 CWND;
	appInt16 prev_CWND;
	appInt16 snd_cwnd_cnt; /* 1 * MSS*/
	appInt16 senderWindow;
	appSInt64 RTT;
	appSInt64 SRTT;
	appSInt64 RTTVAR;
	appSInt64 RTO;
	appSInt8 K;
	appSInt8 mulAlpha;
	appSInt8 scaleAlpha;
	appSInt8 mulBeta;
	appSInt8 scaleBeta;

	appInt16 ssthresh;
	appInt fast_convergence = 1;
	appInt beta = 717; /* = 717/1024 (BICTCP_BETA_SCALE) */
	appInt bic_scale = 41;
	appInt tcp_friendliness = 1;
	appInt32 cube_rtt_scale;
	appInt32 beta_scale;
	appInt64 cube_factor;
	std::mutex write_mtx;
	std::mutex mtx_rcv_buffer;
	std::mutex mtx_packet_rcv;

};

#endif /* SRC_TUNNELLIB_CUBICCHANNELHANDLER_HPP_ */
