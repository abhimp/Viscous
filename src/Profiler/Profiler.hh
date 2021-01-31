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


#ifndef __PROFILER_H__
#define __PROFILER_H__

//#define __PROFILER_ENABLED__

#ifdef __PROFILER_ENABLED__

#define __PROFILE_PROF_TIME_ENABLED__

#include<unistd.h>
#include<ctime>
#include<unordered_map>
#include<common.h>
#include<memory>

#define TO_STR(x) #x

namespace Profiler{
typedef size_t id_t;
class Pinstance;
class PacketProfile;

Pinstance *findOrCreate(const char *name, const char *location);
PacketProfile *findOrCreatePktPrf(const char *loc, const char *fun);
void dump();
void reset();

const uint64_t SEC_NSEC = 1000000000L;

#ifdef __PROFILE_PROF_TIME_ENABLED__
class ProfTime{
public:
    ProfTime(uint64_t x=0):tv_sec(0), tv_nsec(0){if(x!=0) *this=x;}
    uint64_t   tv_sec;
    uint64_t  tv_nsec;
    void operator=(uint64_t x){
        tv_nsec = x%SEC_NSEC;
        tv_sec  = x/SEC_NSEC;
    }
    void operator=(const ProfTime &pt){
        tv_nsec = pt.tv_nsec;
        tv_sec  = pt.tv_sec;
    }
    ProfTime &operator+=(const ProfTime &pt){
        this->tv_nsec += pt.tv_nsec;
        this->tv_sec  += pt.tv_sec;
        if(tv_nsec > SEC_NSEC){
            tv_sec ++;
            tv_nsec -= SEC_NSEC;
        }
        return *this;
    }

    ProfTime operator-(const ProfTime &pt){
    	ProfTime tmp;
        if(tv_nsec < pt.tv_nsec){
            tmp.tv_nsec = (tv_nsec+SEC_NSEC) - pt.tv_nsec;
            tmp.tv_sec = tv_sec - pt.tv_sec - 1;
        }
        else{
            tmp.tv_nsec = tv_nsec - pt.tv_nsec;
            tmp.tv_sec  = tv_sec -pt.tv_sec;
        }
        return tmp;
    }
};
inline ProfTime getTime(){ProfTime y; timespec x; clock_gettime(CLOCK_REALTIME_COARSE, &x); y.tv_nsec = x.tv_nsec; y.tv_sec = x.tv_sec; return y;}
#else
//typedef clock_t ProfTime;
class ProfTime{
public:
    ProfTime(uint64_t x=0):tm(x){}
    clock_t   tm;
    void operator=(uint64_t x){
    	tm = x;
    }
    void operator=(const ProfTime &pt){
        tm = pt.tm;
    }
    ProfTime &operator+=(const ProfTime &pt){
    	tm += pt.tm;
        return *this;
    }

    ProfTime operator-(const ProfTime &pt){
    	ProfTime tmp;
    	tmp.tm = tm - pt.tm;
        return tmp;
    }
};
//inline ProfTime getTime(){return clock();}
inline ProfTime getTime(){ProfTime y; y = clock() ; return y;}
#endif //__PROFILE_PROF_TIME_ENABLED__


class Pinstance{
private:
    const char *location;
    const char *name;
    id_t id;
    uint32_t cnt;
    clock_t ticks;
    clock_t beg;
public:
    Pinstance(const char *name, const char *l):location(l), name(name), id((id_t)name), cnt(0), ticks(0), beg(0){}
    Pinstance(const char *name):location("UNKNOWN LOCATION"), name(name), id((id_t)name), cnt(0), ticks(0), beg(0){}
    Pinstance(const char *name, id_t id):location("UNKNOWN LOCATION"), name(name), id(id), cnt(0), ticks(0), beg(0){}
    virtual ~Pinstance(){}
    void start(){beg = clock(); cnt++;} // it is meaningless to instanciate again
    void stop(){ticks += clock() - beg;}
    uint32_t getCnt() const { return cnt; }
    id_t getId() const { return id; }
    const char* getName() const { return name; }
    clock_t getTicks() const { return ticks; }
    void printStat(){fprintf(stderr, "%s:: %s:%lu-> counts: %u, ticks %lu\n", location, name, id, cnt, ticks);}
};

class PacketProfile{
private:
    const char *fun;
    const char *location;
    id_t id;
    uint32_t cnt;
    ProfTime ticks;
public:
    PacketProfile(const char *loc, const char *fun): fun(fun), location(loc), id((id_t)loc), cnt(0), ticks(0){}
    virtual ~PacketProfile(){}
    void add(ProfTime interval){ticks += interval; cnt++;}
#ifdef __PROFILE_PROF_TIME_ENABLED__
    void printStat(){fprintf(stderr, "%s::%s::%lu-> counts: %u, ticks %lu.%09lu\n", location, fun, id, cnt, ticks.tv_sec, ticks.tv_nsec);}
#else
    void printStat(){fprintf(stderr, "%s::%s::%lu-> counts: %u, ticks %lu\n", location, fun, id, cnt, ticks.tm);}
#endif

    uint32_t getCnt() const { return cnt; }
    id_t getId() const { return id; }
    const char* getLocation() const { return location; }
    ProfTime getTicks() const { return ticks; }
};

class PerPacketProfile{
	private:
	std::unordered_map<id_t, ProfTime> timeMap;
	const char *lastId;
	ProfTime lastTime;
	public:
	PerPacketProfile(): timeMap(), lastId(NULL), lastTime(0) {}
	~PerPacketProfile();
	void startWaiting(const char *loc);
	void stopWaiting(ProfTime &now, const char *loc);
};

class FlowTiming{
private:
	const char *msg;
	appInt32 flowId;
	appInt32 seqId;
	appInt32 cnt;
	ProfTime time;
public:
	FlowTiming(const char *msg, appInt32 flowId, appInt32 seqId, appInt32 cnt, ProfTime time);
	~FlowTiming();
	void print();
};

class FlowTimingScoped{
private:
	const char *msg;
	appInt32 flowId;
	appInt32 seqId;
	appInt32 cnt;
	ProfTime started;
public:
	FlowTimingScoped(const char *msg, appInt32 flowId, appInt32 seqId, appInt32 cnt);
	~FlowTimingScoped();
};

class Scoped{
    Pinstance *p;
public:
    Scoped(const char *funName, const char *l):p(findOrCreate(funName, l)){p->start();}
    ~Scoped(){p->stop();}
};

};
//#define TOLKOPA(x) x
#define CONCATE(x,y) x##y
#define IS_THERE(map, key) (map.find(key) != map.end())
#define FLOW_SCOPED(x, y, z, w) Profiler::FlowTimingScoped p(x, y, z, w)
#define PROFILE_SCOPED_PRI(fun_, file_, line_) Profiler::Scoped CONCATE(profscoped,line_) (fun_, file_ ":" TO_STR(line_));
#define PROFILE_SCOPED PROFILE_SCOPED_PRI(__PRETTY_FUNCTION__, __FILE__, __LINE__)
#define PROFILER_DUMP Profiler::dump();
#define PROFILER_RESET Profiler::reset();
#define PROFILE_STANDALONE_START_DUMMY(name, fun_, file_, line_) Profiler::Pinstance *name = Profiler::findOrCreate(fun_, file_ ":" TO_STR(line_)); name->start();
#define PROFILE_STANDALONE_START(name) PROFILE_STANDALONE_START_DUMMY(name, __PRETTY_FUNCTION__, __FILE__, __LINE__)
#define PROFILE_STANDALONE_STOP(name) name->stop();

#define PROFILE_PERPACKET_WAIT_START_DUMMY(prof, file_, line_) { (prof)->startWaiting(file_ ":" TO_STR(line_));}
#define PROFILE_PERPACKET_WAIT_STOP_DUMMY(prof, now, file_, line_) {(prof)->stopWaiting(now, file_ ":" TO_STR(line_));}

#define PROFILE_PACKET_WAIT_UPDATE_DUMMY(ticks, prof, fun_, file_, line_) {auto y = Profiler::getTime() - ticks; Profiler::findOrCreatePktPrf(file_ ":" TO_STR(line_), fun_)->add(y); }
#define PROFILE_PACKET_WAIT_UPDATE(ticks, prof) PROFILE_PACKET_WAIT_UPDATE_DUMMY(ticks, prof, __PRETTY_FUNCTION__, __FILE__, __LINE__);

#else //__PROFILER_ENABLED__

#define CONCATE(x,y)
#define FLOW_SCOPED(msg, flid, seq, _)
#define IS_THERE(map, key) (map.find(key)
#define PROFILE_SCOPED_PRI(fun_, file_, line_)
#define PROFILE_SCOPED
#define PROFILER_DUMP
#define PROFILER_RESET
#define PROFILE_STANDALONE_START_DUMMY(name, fun_, file_, line_)
#define PROFILE_STANDALONE_START(name)
#define PROFILE_STANDALONE_STOP(name)

#define PROFILE_PACKET_WAIT_UPDATE_DUMMY(ticks, file_, line_)
#define PROFILE_PACKET_WAIT_UPDATE(ticks)

#endif //__PROFILER_ENABLED__

#endif // __PROFILER_H__
