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
 * Streamer.h
 *
 *  Created on: 23-Feb-2017
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_FLOWHANDLER_STREAMER_HH_
#define SRC_TUNNELLIB_FLOWHANDLER_STREAMER_HH_
#include "../CommonHeaders.hh"
namespace FlowHandler {

class Streamer {
private:
    ReliabilityMod *obj;
    appFlowIdType flowId_;
    appBool closed;
public:
    Streamer(ReliabilityMod *streamObj, appFlowIdType flowId);
    virtual ~Streamer();
    inline appSInt readData(appByte *data, appInt size);
    inline appSInt sendData(appByte *data, appInt dataLen);
    inline appStatus close();
//    void initiateClosure();
    inline ReliabilityMod *getObj(void){return obj;};
    inline appFlowIdType flowId(){return flowId_;}
    void setCallBack(ReliabilityMod::EventType evt, void *info, void* func);
};

//inline void Streamer::initiateClosure() {
////    obj->initiateClosure();
//}

inline void Streamer::setCallBack(ReliabilityMod::EventType evt, void *info, void* func){
    obj->setCallBack(evt, info, func);
}

inline appSInt Streamer::readData(appByte* data, appInt size) {
    if(closed)
        return -1;
    return obj->readData(flowId_, data, size);
}

inline appSInt Streamer::sendData(appByte* data, appInt dataLen) {
    if(closed)
        return -1;
    return obj->sendData(flowId_, data, dataLen);
}

inline appStatus Streamer::close() {
    if(closed)
        return APP_SUCCESS;
    return obj->closeFlow(flowId_);
}

} /* namespace FlowHandler */

#endif /* SRC_TUNNELLIB_FLOWHANDLER_STREAMER_HH_ */
