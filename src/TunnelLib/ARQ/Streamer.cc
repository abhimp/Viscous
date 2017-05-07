/*
 * Streamer.cpp
 *
 *  Created on: 23-Feb-2017
 *      Author: abhijit
 */

#include "Streamer.h"

namespace ARQ {

Streamer::Streamer(ReliabilityMod* streamObj, appInt16 flowId): obj(streamObj), flowId_(flowId), closed(FALSE){
}

Streamer::~Streamer() {
}

} /* namespace ARQ */
