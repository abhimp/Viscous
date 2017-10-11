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
 * InterfaceMonitor.cc
 *
 *  Created on: 31-Aug-2017
 *      Author: abhijit
 */

#include "Addresses.hh"
#include "SearchMac.hh"
#include "SendThroughInterface.h"
#include "InterfaceMonitor.hh"

//#include <stdio.h>
//#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <common.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <arpa/inet.h>

#define hasKey(_map, _key) (_map.find(_key) != _map.end())

InterfaceMonitor::InterfaceMonitor(appInt localPort): localPort(localPort), nextInterfaceId(0)
{
    memset(interfaceSender, 0, sizeof(interfaceSender));
}

InterfaceMonitor::~InterfaceMonitor() {
//    for(auto x : children){
//        delete x;
//    }
}

void InterfaceMonitor::run() {
    listenOnNetworkEventHack();
}

int InterfaceMonitor::listenOnNetworkEventHack() {
    char buf[1024];
    auto pid = getpid();
    std::string dirname = "/tmp/viscous_proc";
    appChar *procInfoDir = (appChar *)getenv("VISCOUS_PROC_INFO_DIR");
    if(procInfoDir and strcmp((const char *)procInfoDir, (const char *)"")){
        dirname = (char *)procInfoDir;
    }

    struct stat st = {0};
    if (stat(dirname.c_str(), &st) == -1) {
        mkdir(dirname.c_str(), 0700);
    }
    std::stringstream stream;
    stream << pid << std::endl;
    std::string fpath;
    stream >> fpath;

    fpath = dirname + "/" + fpath;

    mkfifo(fpath.c_str(), 0600);

    while(1){

        if(stat(fpath.c_str(), &st) == -1)
            mkfifo(fpath.c_str(), 0600);

        auto fd = open(fpath.c_str(), O_RDONLY);
        int len = read(fd, buf, sizeof(buf));
        buf[len] = 0;

        char *svptr = NULL;
        std::vector<char*> list;
        for(auto x=buf; ; x=NULL){
            auto tok = strtok_r(x, " ", &svptr);
            if(!tok)
                break;
            list.push_back(tok);
        }
        close(fd);

        if(list.size() < 4)
            continue;
        in_addr_t ipaddr = inet_addr(list[2]);
        if(strcmp(list[0], "ifup") == 0){
            populateInterfaces();
            if(!hasKey(ip2InterfaceIdMap, ipaddr)){
                LOGE("Unknown ipaddr %s\n", inet_ntoa(*(in_addr *)&ipaddr));
                continue;
            }
            auto ifcId = ip2InterfaceIdMap[ipaddr];
            if(interfaceSender[ifcId] == 0){
                LOGE("Unknown ipaddr %s\n", inet_ntoa(*(in_addr *)&ipaddr));
                continue;
            }
            for(auto x: children){
                x->AnyEvent();
                x->interfaceAdded(ifcId);
            }

        }
        else if(strcmp(list[0], "ifdown") == 0){
            if(!hasKey(ip2InterfaceIdMap, ipaddr)){
                LOGE("Unknown ipaddr %s\n", inet_ntoa(*(in_addr *)&ipaddr));
                continue;
            }
            auto ifcId = ip2InterfaceIdMap[ipaddr];
            for(auto x: children){
                x->AnyEvent();
                x->interfaceRemoved(ifcId);
            }
            removeInterface(ifcId);
        }

    }
    return 0;
}

int InterfaceMonitor::listenOnNetworkEvent() {
    struct sockaddr_nl addr;
    int sock, len;
    char buffer[4096];
    struct nlmsghdr *nlh;

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
            struct ifaddrmsg *ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);
            struct rtattr *rth = IFA_RTA(ifa);
            int rtl = IFA_PAYLOAD(nlh);

            while (rtl && RTA_OK(rth, rtl)) {
                if (rth->rta_type == IFA_LOCAL) {
//                    uint32_t ipaddr = htonl(*((uint32_t *)RTA_DATA(rth)));
                    uint32_t ipaddr = *((uint32_t *)RTA_DATA(rth));
                    if (nlh->nlmsg_type == RTM_NEWADDR) {
                        populateInterfaces();
                        if(!hasKey(ip2InterfaceIdMap, ipaddr)){
                            continue;
                        }
                        auto ifcId = ip2InterfaceIdMap[ipaddr];
                        if(interfaceSender[ifcId] == 0)
                            continue;
                        for(auto x: children){
                            x->AnyEvent();
                            x->interfaceAdded(ifcId);
                        }
                    }
                    else if (nlh->nlmsg_type == RTM_DELADDR) {
                        if(!hasKey(ip2InterfaceIdMap, ipaddr)){
                            continue;
                        }
                        auto ifcId = ip2InterfaceIdMap[ipaddr];
                        for(auto x: children){
                            x->AnyEvent();
                            x->interfaceRemoved(ifcId);
                        }
                        removeInterface(ifcId);
                    }
                    else{
                        LOGD("unknown event: %d\n", nlh->nlmsg_type);
                    }
                }
                rth = RTA_NEXT(rth, rtl);
            }
            nlh = NLMSG_NEXT(nlh, len);
        }
    }
    return 0;
}

void InterfaceMonitor::attach(NetworkEventListner* ne) {
    assert(ne);
    if(!ne)
        return;
    children.insert(ne);
}

void InterfaceMonitor::detach(NetworkEventListner* ne) {
    if(!ne or !hasKey(children, ne))
        return;
    children.erase(ne);
}

void InterfaceMonitor::start() {
    populateInterfaces();
    util::AppThread::start();
}

void InterfaceMonitor::populateInterfaces() {
    auto list = SearchMac::getIpMatrix();
//    auto cnt = 0;
    if(list.size() > 0){
        for(auto it : list){
            appInt32 ip = it.ifcIp.s_addr;
            appInt8 interfaceId = 0;
            if(hasKey(ip2InterfaceIdMap, ip))
            {
                interfaceId = ip2InterfaceIdMap[ip];
                if(interfaceSender[interfaceId])
                    continue;
            }
            else{
                interfaceId = getNextInterfaceId();
                ip2InterfaceIdMap[ip] = interfaceId;
            }
            it.localPort = htons(localPort);
            SendThroughInterface *tmp = new SendThroughInterface(&it);
            tmp->init();
//            cnt ++;
            interfaceSender[interfaceId] = tmp;
//            getInterfaceSender()[cnt] = tmp; //AS
        }
    return;
    }
}

void InterfaceMonitor::removeInterface(appInt8 ifcId) {
    if(interfaceSender[ifcId] == NULL)
        return;
    auto tmp = interfaceSender[ifcId];
    interfaceSender[ifcId] = NULL;
    delete tmp;
}

appInt8 InterfaceMonitor::getPrimaryInterfaceId() {
    for(appInt8 ifcId = 0; ifcId < 16; ifcId++){
        if(interfaceSender[ifcId])
            return ifcId;
    }
    return 0;
}

appInt8 InterfaceMonitor::getNextInterfaceId() {
    nextInterfaceId++;
    return nextInterfaceId;
}
