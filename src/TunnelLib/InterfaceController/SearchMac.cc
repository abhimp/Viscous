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


#include "SearchMac.hh"

#include <cstdlib>
#include <sys/ioctl.h>
#include <unistd.h>
#include "ValueType.hh"
#include <dirent.h>
#include <fstream>
//#include <iostream>
namespace SearchMac{

#define MAC_LENGTH 6

std::set<std::string> readdir(char *path);
//This is a alternative hack
interfaceIPGWMap getAllGatewayAndIfaceUsingIfHack(){
    std::string path = "/tmp/viscous_ifconf/";
    appChar *networkInfoDir = (appChar *)getenv("VISCOUS_NET_INFO_DIR");
    if(networkInfoDir and strcmp((const char *)networkInfoDir, (const char *)"")){
        path = (char *)networkInfoDir;
    }
    interfaceIPGWMap interfaces;
    auto dirs = readdir((char *)path.c_str());
    char *key = NULL;
    size_t cnt;
    for(auto file:dirs){
        std::string fpath = path + file;
//        std::ifstream fp(fpath);
        if(path.length() > 0 and path[path.length()-1] != '/'){
            fpath = path + "/" + file;
        }
        auto fp = fopen((char *)fpath.c_str(), "r");
        if(!fp)
            continue;
        in_addr_t gw = 0;
        in_addr_t ip = 0;
        std::string iface;
        while(1){
            key = NULL;
            cnt = 0;
            auto len = getline(&key, &cnt, fp);
            if(len < 0)
                break;
            if(len == 0)
                continue;
            if(key[len-1] == '\n')
                key[len-1] = 0;
            else
                key[len] = 0;
//            std::string lr(line);
            auto val = strchr(key, '=');
            *val = 0;
            val++;
            if(strcmp(key, "iface") == 0){
                iface = val;
//                new appChar[strlen(val) + 1];
//                strcpy((char *)iface, val);
            }
            else if(strcmp(key, "gw") == 0){
                gw = inet_addr(val);
            }
            else if(strcmp(key, "ip") == 0){
                ip = inet_addr(val);
            }
//            printf("key=%s, val=%s\n", line, val);
        }
        if(!ip or !gw or iface.empty()){
//            if(iface)
//                delete iface;
            continue;
        }
        std::pair<std::string, in_addr_t> p(iface, gw);
        interfaces[ip] = p;

    }
    return interfaces;
}

interfaceIPMap getAllGatewayAndIface()
{
    long destination, gateway;
    char iface[IF_NAMESIZE];
    char buf[BUFFER_SIZE];
    std::string tmp;
    in_addr_t addr;
    interfaceIPMap interfaces;
    FILE * file;

    memset(iface, 0, sizeof(iface));
    memset(buf, 0, sizeof(buf));

    file = fopen("/proc/net/route", "r");
    if (!file)
        return interfaces;

    while (fgets(buf, sizeof(buf), file)) {
        if (sscanf(buf, "%s %lx %lx", iface, &destination, &gateway) == 3) {
            if (destination == 0) { /* default */
                addr = gateway;
//                tmp = new appChar[strlen(iface) + 1];
//                strcpy((char *)tmp, iface);
                tmp = iface;
                interfaces[tmp] = addr;
            }
        }
    }

    /* default route not found */
    if (file)
        fclose(file);
    return interfaces;
}

int getGatewayAndIface(std::string interface, in_addr_t &addr)
{
    appSInt32 destination, gateway;
    char iface[IF_NAMESIZE];
    char buf[BUFFER_SIZE];
    FILE * file;

    memset(iface, 0, sizeof(iface));
    memset(buf, 0, sizeof(buf));

    file = fopen("/proc/net/route", "r");
    if (!file)
        return -1;

    while (fgets(buf, sizeof(buf), file)) {
        if (sscanf(buf, "%s %x %x", iface, &destination, &gateway) == 3) {
            if (destination == 0 && interface == iface) { /* default */
                addr = gateway;
                fclose(file);
                return 0;
            }
        }
    }

    /* default route not found */
    if (file)
        fclose(file);
    return -1;
}

int getMac(std::string searchIp, std::string &chwaddr, struct ether_addr &hwaddr){
    FILE *fp;
    char buf[BUFFER_SIZE];
    char ip[CONT_LEN], hwt[CONT_LEN], flg[CONT_LEN], hwa[CONT_LEN];
    struct ether_addr *tmaddr;

    fp = fopen("/proc/net/arp", "r");
    if(fp == NULL){
        perror("fread: ");
    }
    
    while(fgets(buf, sizeof(buf), fp)){
        if(sscanf(buf, "%s %s %s %s", ip, hwt, flg, hwa) == 4){
            if(ip == searchIp){
//                if(!chwaddr.empty()){
                    chwaddr = hwa;
//                }
//                if(hwaddr != NULL){
                    tmaddr = ether_aton(hwa);
                    hwaddr = *tmaddr;
//                }
                //printf("%s -> %s\n", ip, hwa);
                fclose(fp);
                return 0;
            }
        }
    }
    fclose(fp);
    return -1;
}

interfaceIPMap getAllIp(){
    struct ifaddrs *addrs, *tmp;
    int status;
    interfaceIPMap ipMap;
    std::string intf;
    status = getifaddrs(&addrs);
    if (status != 0){
        return ipMap;
    }

    in_addr_t invalidMask = inet_addr("255.255.0.0");
    in_addr_t invalidNetwork = inet_addr("169.254.0.0");


    tmp = addrs;

    while (tmp)
    {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            if((invalidNetwork&invalidMask) != (pAddr->sin_addr.s_addr&invalidMask)){
                intf = tmp->ifa_name;
                assert(ipMap.find(intf) == ipMap.end());
//                intf = new char[strlen(tmp->ifa_name) + 1];
//                strcpy(intf, tmp->ifa_name);
                ipMap[intf] = pAddr->sin_addr.s_addr;
            }
        }
        tmp = tmp->ifa_next;
    }

    freeifaddrs(addrs);
    return ipMap;
}

int getMacR(std::string ifc, std::string searchIp, std::string &c_hwaddr, struct ether_addr &e_hwaddr){
    char arpingcmd[100];
    if(getMac(searchIp, c_hwaddr, e_hwaddr) != 0){
        sprintf(arpingcmd, "ping -c 2 -I %s %s", ifc.c_str(), searchIp.c_str());
        system(arpingcmd);
        if(getMac(searchIp, c_hwaddr, e_hwaddr) != 0){
            return 2;
        }
    }
    return 0;
}

int getGatewayAddress(std::string iface, in_addr_t &a_ip, std::string s_ip, struct ether_addr &e_hwaddr, std::string c_hwaddr){
    in_addr_t addr = 0;
    std::string tmp;
    std::string ip;
    if(getGatewayAndIface(iface, addr) != 0){
        fprintf(stderr, "%s: no such device\n", iface);
        return 1;
    }
    a_ip = addr;
//    if(a_ip){
//        *a_ip = addr;
//    }
    tmp = inet_ntoa(*(struct in_addr *) &addr);
//    strcpy(ip, tmp);
//    if(s_ip){
//        strcpy(s_ip, tmp);
//    }
    s_ip = tmp;
    ip = tmp;
    return getMacR(iface, ip, c_hwaddr, e_hwaddr);
}

std::vector< InterfaceAddr > getIpMatrixUsingRoute(void){
    SearchMac::interfaceIPMap intf2gw;
//    std::map<char *, SendThroughInterface*> intf2send;
    SearchMac::interfaceIPMap intf2ip;

    std::vector< InterfaceAddr > list;
    ether_addr e_addr;
    std::string s_addr;

    intf2gw = SearchMac::getAllGatewayAndIface();
    intf2ip = SearchMac::getAllIp();

    for (auto it = intf2gw.begin(); it != intf2gw.end(); ++it) { // calls a_map.begin() and a_map.end()
            std::string ip = inet_ntoa(*(struct in_addr *) &(it->second));
            if(SearchMac::getMacR(it->first, ip, s_addr, e_addr) == 0)
            if(intf2ip.find(it->first) != intf2ip.end()){
                InterfaceAddr s(it->first, *(struct in_addr *) &(it->second), e_addr, *(struct in_addr *) &(intf2ip[it->first]), 0);
                list.push_back(s);

            }
        }
    return list;
}

std::vector<InterfaceAddr> getIpMatrixUsingHack(void){
    interfaceIPGWMap ipIntf;
    ipIntf = getAllGatewayAndIfaceUsingIfHack();
    ether_addr e_addr;
    std::string s_addr;
    std::vector< InterfaceAddr > list;
    for(auto ifc:ipIntf){
        auto ip = ifc.first;
        auto iface = ifc.second.first;
        auto gw = ifc.second.second;

        auto gwips = inet_ntoa(*((struct in_addr *) &gw));
        if(getMacR(iface, gwips, s_addr, e_addr) == 0){
            InterfaceAddr s(iface, *((struct in_addr *) &gw), e_addr, *((struct in_addr *) &ip), 0);
            list.push_back(s);
        }
    }
    return list;
}

std::vector< InterfaceAddr > getIpMatrix(void){
    auto list = getIpMatrixUsingHack();
    if(list.size() == 0){
        list = getIpMatrixUsingRoute();
    }
    return list;
}

/*
 * Gets interface information by name:
 * IPv4
 * MAC
 * ifindex
 */
appInt get_if_info(const char *ifname, in_addr *ip, appByte *mac, int *ifindex)
{
    struct ifreq ifr;
    appInt ret = APP_IF_OTHER_ERROR;
    int sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (sd <= 0) {
//        perror("socket()");
        goto out;
    }
    if (strlen(ifname) > (IFNAMSIZ - 1)) {
//        printf("Too long interface name, MAX=%i\n", IFNAMSIZ - 1);
        goto out;
    }

    strcpy(ifr.ifr_name, ifname);

    if (ioctl(sd, SIOCGIFINDEX, &ifr) == -1) {
        ret |= APP_IF_NO_INDEX;
    }
    *ifindex = ifr.ifr_ifindex;
//    printf("interface index is %d\n", *ifindex);

    //Get MAC address of the interface
    if (ioctl(sd, SIOCGIFHWADDR, &ifr) == -1) {
        ret |= APP_IF_NO_MAC;
    }

    //Copy mac address to output
    memcpy(mac, ifr.ifr_hwaddr.sa_data, MAC_LENGTH);

    if (ioctl(sd, SIOCGIFADDR, &ifr) == -1) {
        ret |= APP_IF_NO_IP;
    }
    if(ifr.ifr_addr.sa_family != AF_INET){
        ret |= APP_IF_NO_IP;
    }
    else{
        *ip = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr;
    }

    if(ioctl(sd, SIOCGIFFLAGS, &ifr) == -1){
        ret |= APP_IF_NO_FLAG;
    }
    ret |= (ifr.ifr_flags & IFF_LOOPBACK) ? APP_IF_LOOPBACK : APP_IF_OK;

    ret &= (~APP_IF_OTHER_ERROR);
out:
    if (sd > 0) {
        close(sd);
    }
    return ret;
}

std::set<std::string> readdir(char *path)
{
    DIR *dp;
    struct dirent *ep;
    std::set<std::string> dirs;
    std::string str;
    dp = opendir(path);
    if(dp != NULL)
    {
        while((ep = readdir(dp)))
        {
            //std::cout << ep->d_name << std::endl;
            str = ep->d_name;
            if(strcmp(ep->d_name, ".")==0 or strcmp(ep->d_name, "..")==0)
                continue;
            dirs.insert(str);
        }
        closedir(dp);
    }
    else
        perror("Couldn't open the directory");
    return dirs;
}
std::set<InterfaceInfo> getInterfaceInfos(void){
    char *netdevclass = (char *)"/sys/class/net/";
    std::set<std::string> dirs = readdir(netdevclass);
    InterfaceInfo ifcInfo;
    std::set<InterfaceInfo> interfaces;
    appInt ret;
    for(auto it: dirs){
        ret = get_if_info(it.c_str(), &ifcInfo.ip, ifcInfo.mac.ether_addr_octet, &ifcInfo.ifindex);
        if(ret)
            continue;
        ifcInfo.ifname = it.c_str();
        interfaces.insert(ifcInfo);
    }
    return interfaces;
}

std::set<InterfaceInfo> getInterfaceAndIp(void){
    ifaddrs *ifa0, *ifa;
    InterfaceInfo ifcInfo;
    std::set<InterfaceInfo> ips;
    int rc = getifaddrs(&ifa0);
    if (rc) {
        perror("getifaddrs");
        return ips;
    }
    for (ifa = ifa0; ifa; ifa = ifa->ifa_next){
        if(!ifa->ifa_addr)
            continue;
        if(ifa->ifa_addr->sa_family != AF_INET)
            continue;
        if(!ifa->ifa_name)
            continue;
        if(ifa->ifa_flags == IFF_LOOPBACK)
            continue;
//        if(ifa->ifa_broadaddr)
//            continue;
//        if (!((struct sockaddr_ll *)ifa->ifa_addr)->sll_halen)
//                    continue;
        ifcInfo.ifindex = if_nametoindex(ifa->ifa_name);
        if(!ifcInfo.ifindex)
            continue;

        ifcInfo.ifname = ifa->ifa_name;
        ifcInfo.ip = ((sockaddr_in *)&ifa->ifa_addr)->sin_addr;
        ips.insert(ifcInfo);
    }
    freeifaddrs(ifa0);
    return ips;
}
}

