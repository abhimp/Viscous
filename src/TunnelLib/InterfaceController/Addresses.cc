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
 * Addresses.cc
 *
 *  Created on: 17-Dec-2016
 *      Author: abhijit
 */

#include <cstring>
#include "Addresses.hh"

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
