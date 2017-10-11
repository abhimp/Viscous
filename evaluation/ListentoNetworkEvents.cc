/*
 * This is an implementation of Viscous protocol.
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
 * listentoNetworkEvents.cc
 *
 *  Created on: 31-Aug-2017
 *      Author: abhijit
 */



#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <common.h>
#include <net/if.h>
#include <unistd.h>
#include "../src/TunnelLib/InterfaceController/InterfaceMonitor.hh"
#include "../src/TunnelLib/InterfaceController/SearchMac.hh"

//interfaceIPVector getAllGatewayAndIfaceUsingIfHack();

namespace test_interfaceMonitor{

class TestNetworkEventListner:public NetworkEventListner{
public:
    TestNetworkEventListner();
    ~TestNetworkEventListner();

    virtual void interfaceAdded(appInt8 ifcId);
    virtual void interfaceRemoved(appInt8 ifcId);

    virtual void print();
    void join();
private:
    InterfaceMonitor ifcMon;
};

}

int testInterFaceMonitor(){
    test_interfaceMonitor::TestNetworkEventListner tnel;
    tnel.join();
    return 0;
}

int listenOnNetworkEventHack() {
    auto pid = getpid();
    std::string dirname = "/tmp/viscous_proc";
    struct stat st = {0};
    if (stat(dirname.c_str(), &st) == -1) {
        mkdir(dirname.c_str(), 0700);
    }
    std::stringstream stream;
    stream << pid << std::endl;
    std::string fpath;
    stream >> fpath;

    fpath = dirname + "/" + fpath;
    char buf[1024];
    mkfifo(fpath.c_str(), 0600);
    while(1){
        auto fd = open(fpath.c_str(), O_RDONLY);
        int len = read(fd, buf, sizeof(buf));
        buf[len] = 0;
        char *svptr = NULL;
        std::vector<char*> list;
        std::cout << buf << std::endl;
        for(auto x=buf; ; x=NULL){
            auto tok = strtok_r(x, " ", &svptr);
            if(!tok)
                break;
            list.push_back(tok);
        }
        close(fd);
    }
    return 0;
}

int listenOnNetworkEvent()
{
    struct sockaddr_nl addr;
    int sock, len;
    char buffer[4096];
    struct nlmsghdr *nlh;
    auto ifaces = SearchMac::getAllGatewayAndIfaceUsingIfHack();
    for(auto x : ifaces){
        auto p = x.second;
        std::cout << x.first << " : " << p.first << ":" << p.second << std::endl;
    }
    return testInterFaceMonitor();
    return listenOnNetworkEventHack();
    if ((sock = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) == -1) {
        perror("couldn't open NETLINK_ROUTE socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV4_IFADDR;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("couldn't bind");
        return 1;
    }

    nlh = (struct nlmsghdr *)buffer;
    while ((len = recv(sock, nlh, 4096, 0)) > 0) {
        while ((NLMSG_OK(nlh, len)) && (nlh->nlmsg_type != NLMSG_DONE)) {
            if (nlh->nlmsg_type == RTM_NEWADDR or nlh->nlmsg_type == RTM_DELADDR) {
                struct ifaddrmsg *ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);
                struct rtattr *rth = IFA_RTA(ifa);
                int rtl = IFA_PAYLOAD(nlh);

                while (rtl && RTA_OK(rth, rtl)) {
                    if (rth->rta_type == IFA_LOCAL) {
                        uint32_t ipaddr = htonl(*((uint32_t *)RTA_DATA(rth)));
                        char name[IFNAMSIZ];
                        if_indextoname(ifa->ifa_index, name);
                        printf("%s is now %d.%d.%d.%d\n",
                                name,
                                (ipaddr >> 24) & 0xff,
                                (ipaddr >> 16) & 0xff,
                                (ipaddr >> 8) & 0xff,
                                ipaddr & 0xff);
                    }
                    rth = RTA_NEXT(rth, rtl);
                }
            }
            else{
                printf("unknown event: %d\n", nlh->nlmsg_type);
            }
            nlh = NLMSG_NEXT(nlh, len);
        }
    }
    return 0;
}

inline test_interfaceMonitor::TestNetworkEventListner::TestNetworkEventListner(): ifcMon(0) {
    ifcMon.start();
    ifcMon.attach(this);
    print();
}

inline void test_interfaceMonitor::TestNetworkEventListner::interfaceAdded(
        appInt8 ifcId) {
    std::cout << "==============" << std::endl;
    std::cout << "Interface added (" << (int)ifcId << ")" << std::endl;
    print();
    std::cout << "==============" << std::endl << std::endl;
}

inline void test_interfaceMonitor::TestNetworkEventListner::interfaceRemoved(
        appInt8 ifcId) {
    std::cout << "==============" << std::endl;
    std::cout << "Interface removed (" << (int)ifcId << ")" << std::endl;
    print();
    std::cout << "==============" << std::endl << std::endl;
}

inline void test_interfaceMonitor::TestNetworkEventListner::print() {
    char *ip;
    ether_addr eadr;
    appChar iface[100];
    for(int x = 1; x < 16; x++){
        auto ifc = ifcMon[x];
        if(!ifc)
            continue;
        std::cout << x << ":\t";
        ifc->getIface(iface, sizeof(iface));
        std::cout << iface << "\t";
        ip = inet_ntoa(ifc->getLocIp());
        std::cout << ip << "<>";
        eadr = ifc->getLocMac();
        ip = ether_ntoa(&eadr);
        std::cout << ip << " <---> ";
        ip = inet_ntoa(ifc->getGwIp());
        std::cout << ip << "<>";
        eadr = ifc->getGwMac();
        ip = ether_ntoa(&eadr);
        std::cout << ip << std::endl;
    }
}

inline test_interfaceMonitor::TestNetworkEventListner::~TestNetworkEventListner() {
    std::cout << "removed" << std::endl;
}

inline void test_interfaceMonitor::TestNetworkEventListner::join() {
    std::cout << "created" << std::endl;
    ifcMon.wait();
}
