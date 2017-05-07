/*
 * Connection.cc
 *
 *  Created on: 07-Aug-2016
 *      Author: abhijit
 */

#include "Connection.hpp"
#include "PacketPool.h"
//#define APP_REC_HEADER
appSInt udpconnectionfd = 0;
//void static datarecv(EV_P_ ev_io *w, int revents){
//	Connection *con = (Connection *)w->data;
//	con->dataRecvCb(loop, w, revents);
//}

inline appInt Connection::getLocalPort(){
    return ntohs(localAddr.sin_port);
}

appStatus Connection::startClient(appInt localPort, appByte *localIp){
	socketFd = app_udp_listener_str(localPort, (char *)localIp);
	if(udpconnectionfd == 0)
	    udpconnectionfd = socketFd;
	localAddrLen = sizeof(localAddr);
	getsockname(socketFd, (struct sockaddr*)&localAddr, &localAddrLen);
	LOGI("waiting for packet at %s:%d\n", inet_ntoa(localAddr.sin_addr), ntohs(localAddr.sin_port));

#ifdef APP_REC_HEADER
	int opt = 1;
    setsockopt(socketFd, IPPROTO_IP, IP_PKTINFO, &opt, sizeof(opt));
#endif

//	watcher.data = this;
//	ev_init(&watcher, datarecv);
//
////	loop = EV_DEFAULT;
//	ev_io_set(&watcher, socketFd, EV_READ);
//	ev_io_start(EV_DEFAULT_ &watcher);
	APP_RETURN_SUCCESS;
}

appStatus Connection::startServer(appInt localPort, appByte *localIp){
	allowNewCon = TRUE;
	socketFd = app_udp_listener_str(localPort, (char *)localIp);
	localAddrLen = sizeof(localAddr);
	getsockname(socketFd, (struct sockaddr*)&localAddr, &localAddrLen);
	LOGI("waiting for packet at port %s:%d\n", inet_ntoa(localAddr.sin_addr), ntohs(localAddr.sin_port));

//	watcher.data = this;
//	ev_init(&watcher, datarecv);
//
//	ev_io_set(&watcher, socketFd, EV_READ);
//	ev_io_start(EV_DEFAULT_ &watcher);
	APP_RETURN_SUCCESS;
}


void Connection::dataRecvCb(EV_P_ ev_io *w, int revents){
	appSInt n;
	appChar msg[2048];
	sockaddr_in src_addr, dest_addr;
	socklen_t len = sizeof(src_addr);

#ifdef APP_REC_HEADER
	msghdr hdr;
	iovec vec[1];
	in_pktinfo inpkt;
	bool have_inpkt = false;
	char msgCntrl[2048];
	cmsghdr *cmsg;

	vec[0].iov_base = msg;
	vec[0].iov_len = sizeof(msg);

	hdr.msg_name = &src_addr;
	hdr.msg_namelen = len;
	hdr.msg_iov = vec;
	hdr.msg_iovlen = 1;
	hdr.msg_control = msgCntrl;
	hdr.msg_controllen = sizeof(msgCntrl);
	hdr.msg_flags = 0;

	n = recvmsg(socketFd, &hdr, 0);
#else
	n=recvfrom(socketFd,msg,sizeof(msg),0,(struct sockaddr*)&src_addr,&len);
#endif
	if(n <= 0){
		LOGI("Some unknown error. Received a data of len %d\n", n);
		return;
	}

	Packet *pkt = decodeHeader((appByte *)msg, n);
	if(!pkt){
		LOGI("Error in header parsing\n");
		return;
	}

#ifdef APP_REC_HEADER
	for (cmsg = CMSG_FIRSTHDR(&hdr); cmsg != 0; cmsg = CMSG_NXTHDR(&hdr, cmsg))
	{
	    if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO)
	    {
	        inpkt = *(struct in_pktinfo*)CMSG_DATA(cmsg);
	        have_inpkt = true;
	        break;
	    }
	}

	dest_addr.sin_port = htons(getLocalPort());
	if(have_inpkt)
	    dest_addr.sin_addr = inpkt.ipi_addr;
	else{
	    dest_addr.sin_addr.s_addr = htonl(pkt->header.destIp);
	}
#endif
	pkt->src_addr = src_addr;
	pkt->dest_addr = dest_addr;
	parent->recvPacketFrom(pkt);
}

void Connection::listen() {
	appSInt n;
	appChar msg[2048];
	sockaddr_in src_addr, dest_addr;
	socklen_t len = sizeof(src_addr);

	while(1){
	    n=recvfrom(socketFd,msg,sizeof(msg),0,(struct sockaddr*)&src_addr,&len);
	    if(n <= 0){
	        perror("sum error");
	        LOGI("Some unknown error. Received a data of len %d\n", n);
	        continue;
	    }

	    Packet *pkt = decodeHeader((appByte *)msg, n);
	    if(!pkt){
	        LOGI("Error in header parsing\n");
	        continue;
	    }
	    pkt->src_addr = src_addr;
	    pkt->dest_addr = dest_addr;
	    LOGI("pkt recvd: flowid:%d, seq:%d ack%d", pkt->header.flowId, pkt->header.seqNo, pkt->header.ackNo);
	    parent->recvPacketFrom(pkt);
	}
}

appSInt Connection::sendPacketTo(appInt id, Packet *pkt, struct sockaddr_in *dest_addr, socklen_t addrlen){
	if(!socketFd)
		return -1;
	if(!dest_addr || !addrlen)
		return -2;
	appChar msg[2048];
	appInt dlen = 0;
	dlen = encodeHeader(pkt, (appByte *)msg, sizeof(msg));
	if(!dlen)
		return 0;
	return sendto(socketFd, msg, dlen, 0, (const sockaddr*)dest_addr, addrlen);
}

void Connection::close(void) {
//    ev_io_stop(EV_DEFAULT_ &watcher);
    ::close(socketFd);
}
