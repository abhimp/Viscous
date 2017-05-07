/*
 * arpResolv.c
 *
 *  Created on: 27-Feb-2017
 *      Author: abhijit
 */
#include <sys/socket.h>
#include <net/if.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>
#include <net/if_arp.h>
#include "arpResolv.h"

char hello[100];
char *printhex(char *x, int xlen){
    int i;
    int index = 0;
    for(i = 0; i < xlen; i++){
//        int p = x[i];
        sprintf(hello + index, "%hhx", *(x++));
        index += 2;
    }
    return hello;
}
void print(struct sockaddr_ll addr){
        printf( "sll_family: %d \n" \
                "sll_protocol: %d \n" \
                "sll_ifindex: %d \n" \
                "sll_hatype: %d \n" \
                "sll_pkttype: %d \n" \
                "sll_halen: %d \n" \
                "sll_addr: %s \n" \
                "==============\n", \
                addr.sll_family,
                addr.sll_protocol,
                addr.sll_ifindex,
                addr.sll_hatype,
                addr.sll_pkttype,
                addr.sll_halen,
                printhex(addr.sll_addr, addr.sll_halen)
              );
}

void sendBroadCast(int s, struct sockaddr_ll me, uint32_t srcIp, uint32_t remoteIp){
#if 1
    unsigned char buf[256];
    struct arphdr *ah = (struct arphdr*)buf;
    unsigned char *p = (unsigned char *)(ah+1);
    struct sockaddr_ll he;
    char broadAddr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    he = me;
    memcpy(&he.sll_addr, broadAddr, 6);

    ah->ar_hrd = htons(me.sll_hatype);
    if (ah->ar_hrd == htons(ARPHRD_FDDI))
        ah->ar_hrd = htons(ARPHRD_ETHER);
    ah->ar_pro = htons(ETH_P_IP);
    ah->ar_hln = me.sll_halen;
    ah->ar_pln = 4;
    ah->ar_op  = htons(ARPOP_REQUEST);

    memcpy(p, &me.sll_addr, ah->ar_hln);
    p+=me.sll_halen;

    memcpy(p, &srcIp, 4);
    p+=4;

    memcpy(p, broadAddr, 6);
    p+=6;

    memcpy(p, &remoteIp, 4);
    p += 4;
    print(he);
    int err = sendto(s, buf, p - buf, 0, (struct sockaddr *)&he, sizeof(he));

    if(err != p - buf){
        printf("Some error %d\n", ah->ar_hln);
        perror("sendto");
    }
    else{
        printf("Success\n");
    }
#endif
}

uint32_t getSrcIp(char *ifname){
    struct ifaddrs *addrs, *tmp;
    int status;
    status = getifaddrs(&addrs);
    if(status){
        perror("getifaddrs");
        exit(__LINE__);
    }
    for(tmp = addrs; tmp; tmp=tmp->ifa_next){
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET && tmp->ifa_name && strcmp(tmp->ifa_name, ifname) == 0)
        {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            return pAddr->sin_addr.s_addr;
        }
    }
    exit(__LINE__);
    return 0;
}

void recv_addr(int s, struct sockaddr_storage me){
    unsigned char packet[4096];
    struct sockaddr_ll from;
    struct sockaddr_storage FROM;
    struct arphdr *ah;
    unsigned char *p;
//    struct in_addr src_ip, dst_ip;

    socklen_t alen = sizeof(FROM);
    int cc;
    if ((cc = recvfrom(s, packet, sizeof(packet), 0, (struct sockaddr *)&FROM, &alen)) < 0) {
        perror("arping: recvfrom");
        exit(__LINE__);
    }
    from = *((struct sockaddr_ll *)&FROM);
    ah = (struct arphdr *)packet;
    p = (char *)(ah+1);
    if (from.sll_pkttype != PACKET_HOST &&
            from.sll_pkttype != PACKET_BROADCAST &&
            from.sll_pkttype != PACKET_MULTICAST){
        printf("unknown packet type");
        return;
    }

    if (ah->ar_op != htons(ARPOP_REQUEST) &&
            ah->ar_op != htons(ARPOP_REPLY)){
        printf("unknown operation type");
        return ;
    }

    /* ARPHRD check and this darned FDDI hack here :-( */
    if (ah->ar_hrd != htons(from.sll_hatype) &&
            (from.sll_hatype != ARPHRD_FDDI || ah->ar_hrd != htons(ARPHRD_ETHER))){
        printf("some ARPHRD and FDDI error");
        return ;
    }

    /* Protocol must be IP. */
    if (ah->ar_pro != htons(ETH_P_IP)){
        printf("unknown proto");
        return;
    }
    if (ah->ar_pln != 4){
        printf("unknown protocol len");
        return ;
    }
    if (ah->ar_hln != ((struct sockaddr_ll *)&me)->sll_halen){
        printf("unknown arplen");
        return ;
    }
    if (cc < sizeof(*ah) + 2*(4 + ah->ar_hln)){
        printf("some error");
        return ;
    }
    //memcpy(&src_ip, p+ah->ar_hln, 4);
    //memcpy(&dst_ip, p+ah->ar_hln+4+ah->ar_hln, 4);

    printf("%s\n", printhex(p+ah->ar_hln + 4, ah->ar_hln));
}

int arpmain(appInt32 srcIp, appInt32 remoteIp, appSInt ifid){
//    unsigned int ifid  = 0;
    struct sockaddr_storage me;
    int s;
//    uint32_t srcIp, remoteIp;
//    srcIp = getSrcIp(argv[1]);
//    ifid = if_nametoindex(argv[1]);
//    remoteIp = inet_addr(argv[2]);

    s = socket(PF_PACKET, SOCK_DGRAM, 0);
    if(s < 0){
        perror("socket: ");
        return __LINE__;
    }
    ((struct sockaddr_ll *)&me)->sll_family = AF_PACKET;
    ((struct sockaddr_ll *)&me)->sll_ifindex = ifid;
    ((struct sockaddr_ll *)&me)->sll_protocol = htons(ETH_P_ARP);
    if(bind(s, (struct sockaddr*)&me, sizeof(me)) == -1) {
        perror("bind");
        return __LINE__;
    }
    {
        int alen = sizeof(me);
        if(getsockname(s, (struct sockaddr*)&me, &alen) == -1){
            perror("getsockname");
            return __LINE__;
        }
    }
    print(*((struct sockaddr_ll *)&me));
    sendBroadCast(s, *((struct sockaddr_ll *)&me), srcIp, remoteIp);
    recv_addr(s, me);
    return 0;
}




