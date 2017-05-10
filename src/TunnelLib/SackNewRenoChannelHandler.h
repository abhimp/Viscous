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
 * SackNewRenoChannelHandler.h
 *
 *  Created on: 26-Dec-2016
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_SACKNEWRENOCHANNELHANDLER_H_
#define SRC_TUNNELLIB_SACKNEWRENOCHANNELHANDLER_H_

#include "BasicChannelHandler.h"
#include "../util/BitField.hpp"
#include <set>
#include "../../util/ConditonalWait.hpp"

#define MAX_CONG_WINDOW (APP_INT16_CNT/2)
#define INITIAL_SSTHRESH (200)//(MAX_CONG_WINDOW/2)
class SackNewRenoChannelHandler: public BasicChannelHandler {
private:
    appInt16 multiRecoverIndex;
    appInt16 recoverIndex;
public:
    SackNewRenoChannelHandler(BaseReliableObj *parent, appInt8 ifcLoc, RemoteAddr &rmadr, appInt8 ifcSrc, appInt8 ifcDst);
    virtual ~SackNewRenoChannelHandler();

	virtual appInt ackRecv(Packet *pkt);
	virtual appInt sendAllTillRecovery();
	virtual void sendSeq(appInt16 seq);
};

#endif /* SRC_TUNNELLIB_SACKNEWRENOCHANNELHANDLER_H_ */
