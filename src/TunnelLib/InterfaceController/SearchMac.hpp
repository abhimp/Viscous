/*
 * searchmac.h
 *
 *  Created on: 26-Nov-2016
 *      Author: abhijit
 */

#ifndef SEARCHMAC_H_
#define SEARCHMAC_H_


#include <stdio.h>
#include <string.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <map>
#include <vector>
#include <set>
#include <ifaddrs.h>
#include <common.h>

#include "ValueType.hpp"
#include "Addresses.hpp"

namespace SearchMac{
const unsigned int BUFFER_SIZE = 4096;
const unsigned int CONT_LEN = 40;

typedef enum _DEF_FD_{
    SEARCH_MAC_TYPE_IFC_NAME = 0,
    SEARCH_MAC_TYPE_GW_IP,
    SEARCH_MAC_TYPE_GW_ADDR,
    SEARCH_MAC_TYPE_IFC_IP,
}SearchMacType;

typedef enum {
    APP_IF_OK = 0,
    APP_IF_OTHER_ERROR = 1<<0,
    APP_IF_LOOPBACK = 1<<1,
    APP_IF_NO_IP = 1<<2,
    APP_IF_NO_INDEX = 1<<3,
    APP_IF_NO_MAC = 1<<4,
    APP_IF_NO_FLAG = 1<<5
}AppIfFlag;

struct cmp_str
{
   bool operator()(const appString a, const appString b)
   {
      return strcmp((char *)a, (char *)b) < 0;
   }
};

typedef std::map<appString, in_addr_t, cmp_str> interfaceIPVector;
typedef std::map<SearchMacType, Value *> ifaceDetail;
interfaceIPVector getAllGatewayAndIface();
interfaceIPVector getAllIp();
int getGatewayAndIface(in_addr_t * addr, char *interface);
int getMac(char *searchIp, char *chwaddr, struct ether_addr *hwaddr);
int getMacR(char *ifc, char *searchIp, char *c_hwaddr, struct ether_addr *e_hwaddr);
int getGatewayAddress(char *iface, in_addr_t *a_ip, char *s_ip, struct ether_addr *e_hwaddr, char *c_hwaddr);
std::vector<InterfaceAddr> getIpMatrix(void);
std::set<InterfaceInfo> getInterfaceInfos(void);

std::set<InterfaceInfo> getInterfaceInfos(void);
std::set<InterfaceInfo> getInterfaceAndIp(void);
}
#endif /* SEARCHMAC_H_ */
