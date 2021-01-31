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
 * InterfaceMonitor.hh
 *
 *  Created on: 31-Aug-2017
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_INTERFACECONTROLLER_INTERFACEMONITOR_HH_
#define SRC_TUNNELLIB_INTERFACECONTROLLER_INTERFACEMONITOR_HH_

#include "../../util/AppThread.hh"
#include "SendThroughInterface.h"
#include <set>
#include <map>

class NetworkEventListner{
public:
    virtual void AnyEvent(){};
    virtual void interfaceAdded(appInt8 ifcId) {}
    virtual void interfaceRemoved(appInt8 ifcId) {}
    virtual ~NetworkEventListner() {};
};

struct RouteEntry{
    in_addr_t destination;
    in_addr_t gateway;
    in_addr_t mask;
    char iface[IF_NAMESIZE];
};

class InterfaceMonitor : public util::AppThread {
public:
    InterfaceMonitor(appInt localPort);
    virtual ~InterfaceMonitor();
    void run();
    int listenOnNetworkEvent();
    int listenOnNetworkEventHack();
    void attach(NetworkEventListner *ne);
    void detach(NetworkEventListner *ne);
    void start();
    void populateInterfaces();
    void removeInterface(appInt8 ifcId);
    inline SendThroughInterface *operator [](appInt8 ifcId);
    inline SendThroughInterface *get(appInt8 ifcId);
    appInt8 getPrimaryInterfaceId();
    appInt8 getSuitableInterfaceId(in_addr_t destinationIp);
    appBool isReachable(appInt8 ifcLocId, in_addr_t remIp);
private:
    pthread_t start(void *){return 0;};
    std::vector<RouteEntry> populateRouteEntries();
    std::set<NetworkEventListner *> children;
    appInt localPort;
    SendThroughInterface *interfaceSender[INTERFACE_SENDER_CNT]; //0 will never used
    std::map<appInt32, appInt8> ip2InterfaceIdMap;
    appInt8 nextInterfaceId;

//    std::vector<RouteEntry> routeEntries;
    appInt8 getNextInterfaceId();
};

inline SendThroughInterface* InterfaceMonitor::operator [](appInt8 ifcId) {
    return interfaceSender[ifcId];
}

inline SendThroughInterface* InterfaceMonitor::get(appInt8 ifcId) {
    return interfaceSender[ifcId];
}

#endif /* SRC_TUNNELLIB_INTERFACECONTROLLER_INTERFACEMONITOR_HH_ */
