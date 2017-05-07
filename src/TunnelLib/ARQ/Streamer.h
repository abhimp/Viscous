/*
 * Streamer.h
 *
 *  Created on: 23-Feb-2017
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_ARQ_STREAMER_H_
#define SRC_TUNNELLIB_ARQ_STREAMER_H_
#include "../CommonHeaders.hpp"
namespace ARQ {

class Streamer {
private:
    ReliabilityMod *obj;
    appInt16 flowId_;
    appBool closed;
public:
    Streamer(ReliabilityMod *streamObj, appInt16 flowId);
    virtual ~Streamer();
    inline appSInt readData(appByte *data, appInt size);
    inline appSInt sendData(appByte *data, appInt dataLen);
//    inline appStatus close();
    void initiateClosure();
    inline ReliabilityMod *getObj(void){return obj;};
    inline appInt16 flowId(){return flowId_;}
    void setCallBack(ReliabilityMod::EventType evt, void *info, void* func);
};

inline void Streamer::initiateClosure() {
    obj->initiateClosure();
}

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

//inline appStatus Streamer::close() {
//    if(closed)
//        return APP_SUCCESS;
//    return obj->closeFlow(flowId_);
//}

} /* namespace ARQ */

#endif /* SRC_TUNNELLIB_ARQ_STREAMER_H_ */
