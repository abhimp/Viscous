/*
 * PacketHijacking.h
 *
 *  Created on: 13-Apr-2017
 *      Author: abhijit
 */

#ifndef TEST_PACKETHIJACKING_H_
#define TEST_PACKETHIJACKING_H_
#include <common.h>

namespace PacketHijack{
void startClient(appByte *serverIp, appInt serverPort, appInt queueNumber);
}



#endif /* TEST_PACKETHIJACKING_H_ */
