/*
 * Addresses.hpp
 *
 *  Created on: 17-Dec-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_INTERFACECONTROLLER_ADDRESSES_HPP_
#define SRC_TUNNELLIB_INTERFACECONTROLLER_ADDRESSES_HPP_
#include <netinet/ether.h>
#include <netinet/in.h>
#include <common.h>
#include <iostream>
#include <arpa/inet.h>

struct RemoteAddr{
public:
	RemoteAddr(in_addr ip, appInt port) : ip(ip), port(port){};
	RemoteAddr(sockaddr_in &addr): ip(addr.sin_addr), port(addr.sin_port){}
	in_addr ip;
	appInt port;
	bool operator<(const RemoteAddr &rAddr) const;
};

struct InterfaceAddr{
    InterfaceAddr():ifc(NULL), localPort(0){}
    InterfaceAddr(appString ifc, in_addr gwIp, ether_addr gwMac, in_addr infcIp, appInt localPort)
        : ifc(ifc), gwIp(gwIp), gwMac(gwMac), ifcIp(infcIp), localPort(localPort){}
    appString ifc;
    in_addr gwIp;
    ether_addr gwMac;
    in_addr ifcIp;
    appInt localPort;
    bool operator<(const InterfaceAddr addr) const;
};

struct InterfaceInfo{
    std::string ifname;
    in_addr ip;
    ether_addr mac;
    appSInt ifindex;
    bool operator<(const InterfaceInfo &other) const{
        return ifindex < other.ifindex;
    }
};

struct InterfaceAndIp{
    std::string ifname;
    in_addr ip;
    ether_addr mac;
    appSInt ifindex;
    bool operator<(const InterfaceAndIp &other) const{
        return ifindex < other.ifindex;
    }
};
std::ostream& operator<<(std::ostream& os, InterfaceInfo& obj);

template<typename T1, typename T2>
struct AppPair{
    T1 t1;
    T2 t2;
    AppPair(T1 &a1, T2 &a2): t1(a1), t2(a2){}
    bool operator<(const AppPair<T1,T2> comp) const{
        if(t1 < comp.t1)
            return true;
        return t2 < comp.t2;
    }
};

#endif /* SRC_TUNNELLIB_INTERFACECONTROLLER_ADDRESSES_HPP_ */
