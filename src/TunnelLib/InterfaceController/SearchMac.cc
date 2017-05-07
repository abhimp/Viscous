#include "SearchMac.hpp"

#include <cstdlib>
#include <sys/ioctl.h>
#include <unistd.h>
#include "ValueType.hpp"
#include <dirent.h>
namespace SearchMac{

#define MAC_LENGTH 6

interfaceIPVector getAllGatewayAndIface()
{
    long destination, gateway;
    char iface[IF_NAMESIZE];
    char buf[BUFFER_SIZE];
    appString tmp;
    in_addr_t addr;
    interfaceIPVector interfaces;
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
                tmp = new appChar[strlen(iface) + 1];
                strcpy((char *)tmp, iface);
                interfaces[tmp] = addr;
            }
        }
    }

    /* default route not found */
    if (file)
        fclose(file);
    return interfaces;
}

int getGatewayAndIface(in_addr_t * addr, char *interface)
{
    long destination, gateway;
    char iface[IF_NAMESIZE];
    char buf[BUFFER_SIZE];
    FILE * file;

    memset(iface, 0, sizeof(iface));
    memset(buf, 0, sizeof(buf));

    file = fopen("/proc/net/route", "r");
    if (!file)
        return -1;

    while (fgets(buf, sizeof(buf), file)) {
        if (sscanf(buf, "%s %lx %lx", iface, &destination, &gateway) == 3) {
            if (destination == 0 && strcmp(interface, iface) == 0) { /* default */
                *addr = gateway;
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

int getMac(char *searchIp, char *chwaddr, struct ether_addr *hwaddr){
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
            if(strcmp(ip, searchIp) == 0){
                if(chwaddr != NULL){
                    strcpy(chwaddr, hwa);
                }
                if(hwaddr != NULL){
                    tmaddr = ether_aton(hwa);
                    *hwaddr = *tmaddr;
                }
                //printf("%s -> %s\n", ip, hwa);
                fclose(fp);
                return 0;
            }
        }
    }
    fclose(fp);
    return -1;
}

interfaceIPVector getAllIp(){
    struct ifaddrs *addrs, *tmp;
    int status;
    interfaceIPVector ipMap;
    char *intf;
    status = getifaddrs(&addrs);
    if (status != 0){
        return ipMap;
    }

    tmp = addrs;

    while (tmp)
    {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            intf = new char[strlen(tmp->ifa_name) + 1];
            strcpy(intf, tmp->ifa_name);
            ipMap[(appString)intf] = pAddr->sin_addr.s_addr;
//            delete[] intf;
        }

        tmp = tmp->ifa_next;
    }

    freeifaddrs(addrs);
    return ipMap;
}

int getMacR(char *ifc, char *searchIp, char *c_hwaddr, struct ether_addr *e_hwaddr){
    char arpingcmd[100];
    if(getMac(searchIp, c_hwaddr, e_hwaddr) != 0){
        sprintf(arpingcmd, "arping -c 2 -I %s %s", ifc, searchIp);
        system(arpingcmd);
        if(getMac(searchIp, c_hwaddr, e_hwaddr) != 0){
            return 2;
        }
    }
    return 0;
}

int getGatewayAddress(char *iface, in_addr_t *a_ip, char *s_ip, struct ether_addr *e_hwaddr, char *c_hwaddr){
    in_addr_t addr = 0;
    char ip[CONT_LEN], *tmp;
    if(getGatewayAndIface(&addr, iface) != 0){
        fprintf(stderr, "%s: no such device\n", iface);
        return 1;
    }
    if(a_ip){
    	*a_ip = addr;
    }
    tmp = inet_ntoa(*(struct in_addr *) &addr);
    strcpy(ip, tmp);
    if(s_ip){
    	strcpy(s_ip, tmp);
    }

    return getMacR(iface, ip, c_hwaddr, e_hwaddr);
}

std::vector< InterfaceAddr > getIpMatrix(void){
    SearchMac::interfaceIPVector intf2gw;
//    std::map<char *, SendThroughInterface*> intf2send;
    SearchMac::interfaceIPVector intf2ip;

    std::vector< InterfaceAddr > list;
    ether_addr e_addr;

    intf2gw = SearchMac::getAllGatewayAndIface();
    intf2ip = SearchMac::getAllIp();

    for (auto it = intf2gw.begin(); it != intf2gw.end(); ++it) { // calls a_map.begin() and a_map.end()
            char *ip = inet_ntoa(*(struct in_addr *) &(it->second));
            if(SearchMac::getMacR((char *)it->first, ip, NULL, &e_addr) == 0)
    //      std::cout << it->first << ", " << ip << " " << ether_ntoa(&e_addr);
                //appString ifc, in_addr gwIp, ether_addr gwMac, in_addr infcIp, appInt localPort
            if(intf2ip.find(it->first) != intf2ip.end()){
                InterfaceAddr s(it->first, *(struct in_addr *) &(it->second), e_addr, *(struct in_addr *) &(intf2ip[it->first]), 0);
//                s->gwIp = it->second;
//                s->gwMac = e_addr;
//                s->ifc = it->first;
//                s->infcIp = intf2ip[it->first];
//                (*s)[SEARCH_MAC_TYPE_IFC_NAME] = (new appStringValue(it->first));
//                (*s)[SEARCH_MAC_TYPE_GW_IP] = (new inAddrValue(*(struct in_addr *) &(it->second)));
//                (*s)[SEARCH_MAC_TYPE_GW_ADDR] = (new etherAddrValue(e_addr));
//                (*s)[SEARCH_MAC_TYPE_IFC_IP] = (new inAddrValue(*(struct in_addr *) &(intf2ip[it->first])));


                list.push_back(s);

            }
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

