/*
 * Created On : 05.12.2016
 * Author	  : Sourav Bhattacharjee
 * IIT Kharagpur
 *
 */

/*
 * sudo mpudp s port
 * sudo mpudp c ip port
 *
 *
 *
 * */
#include<stdint.h>
#include <string.h>
#include <vector>

#ifndef SRC_TUNNELLIB_CONGESTIONhANDLER_HPP_
#define SRC_TUNNELLIB_CONGESTIONhANDLER_HPP_

#define MAXSENDBUFFER 65536	/* 0 to 65535 */
#define MAXRCVBUFFER 65536	/* 0 to 65535 */
#define MAXSEQNUMBER 65536	/* 0 to 65535 */

struct sender_buffer {
	sender_buffer() :
			lastByteAcked(-1), lastByteWritten(0), lastByteSent(0), indicator(
					false), lastDuplicateAck(0), countDuplicateAck(0) {
		memset(buffer, 0, sizeof(buffer));
		memset(ACK_buffer, true, sizeof(ACK_buffer));
		memset(Timeout_buffer, -1, sizeof(Timeout_buffer));
	}
	struct Packet *buffer[MAXSENDBUFFER];
	bool ACK_buffer[MAXSENDBUFFER];
	appSInt64 Timeout_buffer[MAXSENDBUFFER];
	appSInt16 lastByteAcked;
	int lastByteWritten;
	int lastByteSent;
	/*Indicates Initial starting of the Buffer.*/
	bool indicator;
	int lastDuplicateAck;
	int countDuplicateAck;
};

struct receiver_buffer {
	receiver_buffer() :
			lastByteRead(0), nextByteExpected(0), lastByteRcvd(0), indicatorLastByteRead(
					0), indicatorNextByteExpected(0), indicatorLastByteRcvd(0), indicator(
					0) {
		memset(buffer, 0, sizeof(buffer));
	}
	appInt16 buffer[MAXRCVBUFFER];
	int lastByteRead;
	int nextByteExpected;
	int lastByteRcvd;
	/*Indicates whether above three fields are in same of 0 or not.*/
	bool indicatorLastByteRead;
	bool indicatorNextByteExpected;
	bool indicatorLastByteRcvd;
	/*Indicates Initial starting of the Buffer.*/
	bool indicator;
};

struct Congestion {
	appInt32 cnt; /* increase cwnd by 1 after ACKs */
	appInt32 last_max_cwnd; /* last maximum snd_cwnd */
	appInt32 loss_cwnd; /* congestion window at last loss */
	appInt32 last_cwnd; /* the last snd_cwnd */
	appInt32 last_time; /* time when updated last_cwnd */
	appInt32 origin_point;/* origin point of bic function */
	appInt32 K; /* time to origin point from the beginning of the current epoch */
	appInt32 delay_min; /* min delay (msec << 3) */
	appInt32 epoch_start; /* beginning of an epoch */
	appInt32 ack_cnt; /* number of acks */
	appInt32 tcp_cwnd; /* estimated tcp cwnd */
	appInt16 unused;
	appInt8 sample_cnt; /* number of samples to decide curr_rtt */
	appInt8 found; /* the exit point is found? */
	appInt32 round_start; /* beginning of each round */
	appInt32 end_seq; /* end_seq of the round */
	appInt32 last_ack; /* last time when the ACK spacing is close */
	appInt32 curr_rtt; /* the minimum rtt of current round */
	appInt32 delayed_ack; /* estimate the ratio of Packets/ACKs << 4 */
#define ACK_RATIO_SHIFT	4
};


#endif
