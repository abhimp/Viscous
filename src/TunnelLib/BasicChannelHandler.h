/*
 * BasicChannelHandler.h
 *
 *  Created on: 26-Dec-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_BASICCHANNELHANDLER_H_
#define SRC_TUNNELLIB_BASICCHANNELHANDLER_H_

#include "ChannelHandler.hpp"
#include "../util/BitField.hpp"
#include <set>
#include "../../util/ConditonalWait.hpp"

#define APP_MAX(x,y) ((x) > (y) ? (x) : (y))
#define APP_ABS(x) ((x) < 0 ? (-1)*(x) : (x))

#define MAX_CONG_WINDOW (APP_INT16_CNT/2)
#define INITIAL_SSTHRESH (200)//(MAX_CONG_WINDOW/2)

class BasicChannelHandler: public ChannelHandler {
public:
//    BasicChannelHandler(BaseReliableObj *parent, InterfaceAddr &intfc, RemoteAddr &rmadr);
//    BasicChannelHandler(BaseReliableObj *parent, InterfaceAddr &intfc, RemoteAddr &rmadr, appInt localPort);
    BasicChannelHandler(BaseReliableObj *parent, appInt8 ifcLoc, RemoteAddr &rmadr, appInt8 ifcSrc, appInt8 ifcDst);
    virtual ~BasicChannelHandler();

	virtual appInt16 getSenderWindowSize();
	virtual appSInt sendPacket(Packet *pkt);
	virtual appSInt recv(Packet *pkt);
	virtual bool haveCell();
	virtual appInt timeoutEvent(appTs time);
	virtual appInt ackRecv(Packet *pkt);
	virtual appInt pktRecv(Packet *pkt);
	virtual appInt sendNew(appInt16 cnt = 1);
	virtual inline bool cansend();
	virtual void sendSeq(appInt16 seq);
	virtual void sendAck(Packet *origPkt, appInt16 ackno);
	virtual inline void addBuffer(Packet *pkt);
	inline void updateRtt(Packet *pkt);
	virtual void waitToMoreSpace();
	virtual void shutDown();
protected:
	struct ReceivingBuffer{
	    appInt16 expectedSeq;
	    appInt16 maxRecvd;
	    appBool recvStarted;
	    BitField buf;
	    ReceivingBuffer(): expectedSeq(0), maxRecvd(APP_INT16_MAX), recvStarted(FALSE), buf(APP_INT16_CNT){}
	} recieverBuff;

	struct SendingBuffer{
	    appInt16 lastAcked;
	    appInt16 maxPacket;
	    appInt16 maxSent;
	    appBool started;
	    appInt64 totalSent, totalSuccessfullySent, totalTimeouts, totalDuplicateAck, totalRetransmitions;
	    struct sbuff{
	        appSInt64 time_;
	        Packet *pkt_;
	        appInt32 rto_;
	        appInt8 retryCnt_;
	        inline appSInt64 effTimer() const{return time_ + rto_;}
	        sbuff():time_(0), pkt_(NULL), rto_(0), retryCnt_(0){}
	        void reset(){time_ = 0; pkt_ = NULL; rto_ = 0; retryCnt_ = 0;}
	    } *buf;
	    sbuff &operator[](appInt16 index){
	        return buf[index];
	    }
	    struct classcomp {
	      bool operator() (const sbuff* lhs, const sbuff* rhs) const
	      {
	          if(lhs->effTimer() == rhs->effTimer()){
	              return lhs->pkt_->header.seqNo < rhs->pkt_->header.seqNo;
	          }
	          return lhs->effTimer() < rhs->effTimer();
	      }
	    };
	    std::set<sbuff *, classcomp> timerList;
	    SendingBuffer(): lastAcked(APP_INT16_MAX), maxPacket(APP_INT16_MAX), maxSent(APP_INT16_MAX), started(FALSE),
	            totalSent(0), totalSuccessfullySent(0), totalTimeouts(0), totalDuplicateAck(0), totalRetransmitions(0), buf(NULL), timerList()
	    {
	        buf = new sbuff[APP_INT16_CNT];
	        memset(buf, 0, sizeof(sbuff)*APP_INT16_CNT);
	    }
	    ~SendingBuffer(){
	        delete[] buf;
	    }
	    inline void startTimer(appInt16 seqNo, appSInt64 time, appInt32 rto){
	        APP_ASSERT(!hasKey(timerList, buf+seqNo))
	        buf[seqNo].time_ = time;
	        buf[seqNo].rto_ = rto;
	        timerList.insert(buf+seqNo);
	    }
	    inline void stopTimer(appInt16 seqNo){
	        APP_ASSERT(hasKey(timerList, buf+seqNo));
	        timerList.erase(buf+seqNo);
	        buf[seqNo].time_ = 0;
	        buf[seqNo].rto_ = 0;
	    }
	    inline void restartTimer(appInt16 seqNo, appSInt64 time){
	        APP_ASSERT(hasKey(timerList, buf+seqNo));
            timerList.erase(buf+seqNo);
            buf[seqNo].time_ = time;
            timerList.insert(buf+seqNo);
	    }
	}senderBuffer;
	appInt16 senderWindow;
	float cwnd;
	appInt16 duplicateCount;
	appInt16 ssthresh;
	appInt16 recover;
	appSInt64 rtt, srtt, rto, rttvar;
	appSInt64 const G;
	const float alpha, beta; //default to .8
	std::mutex packetLock;
	enum sender_status{
	    SLOW_START,
	    CONG_AVOIDANCE,
	    FAST_RETRANSMIT,
	    FAST_RECOVERY,
	    MULTI_RECOVERY,
	    TIME_OUT
	} status;
	enum cwnd_update_type{
	    MULTIPLICATIVE_INCREASE, //slow start increase
	    ADDITIVE_INCREASE, //congestion avoidance
	    SET_TO_ONE,
	    SET_TO
	};
	std::mutex sendSeqLock;
	ConditionalWait waitToEmptySapce;
	ConditionalWait waitToIdle;
	appBool closed;
	appBool deadChannel;
	appBool started;
	virtual inline void updateCwnd(cwnd_update_type type, float value = 0.);
	static void* forwardPacketsToIfcScheduler(void *data);
	virtual void sendCSNPacket();
};


inline bool BasicChannelHandler::cansend() {
    appInt16 wnd, pktcnt;
    wnd = senderWindow < (appInt16)(cwnd + 0.5) ? senderWindow : (appInt16)(cwnd + 0.5);
    pktcnt = senderBuffer.maxSent - senderBuffer.lastAcked;
    return pktcnt < wnd;
}

inline bool BasicChannelHandler::haveCell(){
//    return cansend();
//    packetLock.lock();
    if(deadChannel || closed || !started){
//        packetLock.unlock();
        return false;
    }
    appInt16 wnd, pktcnt;
    wnd = senderWindow < (appInt16)(cwnd + 0.5) ? senderWindow : (appInt16)(cwnd + 0.5);
    pktcnt = senderBuffer.maxPacket - senderBuffer.lastAcked;
    auto ret = (pktcnt < wnd*2);
//    packetLock.unlock();
    return ret;
}

inline void BasicChannelHandler::addBuffer(Packet* pkt) {
        senderBuffer.maxPacket ++;
        auto x = senderBuffer.buf + senderBuffer.maxPacket;
        APP_ASSERT(x->pkt_ == NULL && x->time_ == 0);
        senderBuffer.buf[senderBuffer.maxPacket].pkt_ = pkt;
        pkt->header.seqNo = senderBuffer.maxPacket;
}

inline void BasicChannelHandler::updateRtt(Packet *pkt) {
    appSInt64 sentTime = pkt->header.sentTS;//senderBuffer.buf[pkt->header.ackNo].time_;
    rtt = getTime().getMicro() - sentTime;
    if(srtt == 0){
        srtt = rtt;
        rttvar = rtt/2;
        rto = srtt + APP_MAX(G, 4*rttvar);
    }
    else{
        rttvar = (1-beta) * rttvar + beta * APP_ABS(srtt - rtt);
        srtt = (1-alpha)*srtt + alpha*rtt;
        rto = srtt + APP_MAX(G, 4*rttvar);
    }
}

inline void BasicChannelHandler::updateCwnd(cwnd_update_type type,
        float value) {
    switch(type){
        case MULTIPLICATIVE_INCREASE:
            cwnd += 1;
            break;

        case ADDITIVE_INCREASE:
            cwnd += 1/cwnd;
            break;

        case SET_TO_ONE:
            cwnd = 1.;
            break;

        case SET_TO:
            cwnd = value;
            break;
    }

    if(cwnd > MAX_CONG_WINDOW)
        cwnd = MAX_CONG_WINDOW;
    if(cwnd < 1.)
        cwnd = 1.;
}

#endif /* SRC_TUNNELLIB_BASICCHANNELHANDLER_H_ */
