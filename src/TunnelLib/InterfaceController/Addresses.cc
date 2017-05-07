/*
 * Addresses.cc
 *
 *  Created on: 17-Dec-2016
 *      Author: abhijit
 */

#include <cstring>
#include "Addresses.hpp"

bool RemoteAddr::operator<(const RemoteAddr &rAddr) const
{
    if(ip.s_addr != rAddr.ip.s_addr)
        return ip.s_addr < rAddr.ip.s_addr;
    if(port != rAddr.port)
        return port < rAddr.port;
    return false;
}

bool InterfaceAddr::operator <(const InterfaceAddr addr) const
{
    if(localPort != addr.localPort)
        return localPort < addr.localPort;
    if(ifcIp.s_addr != addr.ifcIp.s_addr)
        return ifcIp.s_addr < addr.ifcIp.s_addr;
    appSInt res = std::memcmp(gwMac.ether_addr_octet, addr.gwMac.ether_addr_octet, 6);
    if(res)
        return res < 0;
    return false;
}

std::ostream& operator<<(std::ostream& os, InterfaceInfo& obj){
     os << obj.ifindex << " ";
     os << obj.ifname << " ";
     os << "ip : " << inet_ntoa(obj.ip) << " ";
     os << "mac : " << ether_ntoa(&obj.mac);
     return os;
}
