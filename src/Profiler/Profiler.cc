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
 * BasicChannelHandler.h
 *
 *  Created on: 10-Jun-2017
 *      Author: abhijit
 */


#include "Profiler.hh"
#include <list>

#ifdef __PROFILER_ENABLED__

namespace Profiler{


std::unordered_map<id_t, Pinstance*> instanceMap;
std::unordered_map<id_t, PacketProfile*> pktProfMap;


std::list<std::unordered_map<id_t, ProfTime> > perPktProfMap;
std::list<std::shared_ptr<FlowTiming> > flowTimes;

Pinstance *findOrCreate(const char *name, const char *location){
    auto id = (id_t)name;
    if(IS_THERE(instanceMap, id)){
        return instanceMap[id];
    }
    auto x = new Pinstance(name, location);
    instanceMap[x->getId()] = x;
    return x;
}

PacketProfile *findOrCreatePktPrf(const char *loc, const char *fun){
    auto id = (id_t)loc;
    if(IS_THERE(pktProfMap, id)){
        return pktProfMap[id];
    }
    auto x = new PacketProfile(loc, fun);
    pktProfMap[x->getId()] = x;
    return x;
}

void dump(){

    fprintf(stderr, "=============\n");
    for (auto x: instanceMap){
        x.second->printStat();
    }

    fprintf(stderr, "=============\n");
    for (auto x: pktProfMap){
        x.second->printStat();
    }
    fprintf(stderr, "=============\n");
    for(auto x : perPktProfMap){
    	for(auto y : x){
#ifdef __PROFILE_PROF_TIME_ENABLED__
    		fprintf(stderr, "PerPacketTiming {\"location\": \"%s\", \"sec\": %lu.%09lu}\n", (char *)y.first, y.second.tv_sec, y.second.tv_nsec);
#else
    		fprintf(stderr, "PerPacketTiming {\"location\": \"%s\", \"ticks\": %lu}\n", (char *)y.first, y.second.tm);
#endif
    	}
    }
    fprintf(stderr, "=============\n");
    for(auto x : flowTimes){
    	x->print();
    }
    fprintf(stderr, "=============\n");
}

void reset(){
    instanceMap.clear();
    pktProfMap.clear();
}


PerPacketProfile::~PerPacketProfile() {
	APP_ASSERT(lastId == NULL)
	perPktProfMap.push_back(this->timeMap);
}

void PerPacketProfile::startWaiting(const char *loc) {
	APP_ASSERT(lastId == NULL);
	lastId = loc;
	lastTime = getTime();
}

void PerPacketProfile::stopWaiting(ProfTime &now, const char *loc) {
	if (!lastId) return;
	APP_ASSERT(lastId != NULL);
	APP_ASSERT(!IS_THERE(this->timeMap, (id_t)lastId));
	ProfTime pt = now - lastTime;
	auto id = (id_t)lastId;
	this->timeMap[id] = pt;
	lastId = NULL;
}

FlowTiming::FlowTiming(const char* msg, appInt32 flowId, appInt32 seqId, appInt32 cnt, ProfTime time): msg(msg), flowId(flowId), seqId(seqId), cnt(cnt), time(time) {
}

FlowTiming::~FlowTiming() {
}

FlowTimingScoped::FlowTimingScoped(const char* msg, appInt32 flowId, appInt32 seqId, appInt32 cnt): msg(msg), flowId(flowId), seqId(seqId), cnt(cnt), started(getTime()) {
}

FlowTimingScoped::~FlowTimingScoped() {
	auto diff = getTime() - started;
	std::shared_ptr<FlowTiming> p(new FlowTiming(msg, flowId, seqId, cnt, diff));
	flowTimes.push_back(p);
}

void FlowTiming::print() {
#ifdef __PROFILE_PROF_TIME_ENABLED__
    		fprintf(stderr, "FlowTiming {\"flowId\": \"%d:%d::%s\", \"bytes\": %d, \"sec\" : %lu.%09lu}\n", flowId, seqId, msg, cnt, time.tv_sec, time.tv_nsec);
#else
    		fprintf(stderr, "FlowTiming {\"flowId\": \"%d:%d::%s\", \"bytes\": %d, \"ticks\" : %lu}\n", flowId, seqId, msg, cnt, time.tm);
#endif
}

}


#endif
