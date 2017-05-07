/*
 * TrafficGenerator.h
 *
 *  Created on: 17-Mar-2017
 *      Author: abhijit
 */

#ifndef TEST_TRAFFICGENERATOR_H_
#define TEST_TRAFFICGENERATOR_H_
#include <common.h>
namespace TrafficGeneratorServer{
void startServer(appInt localPort, appByte *fylePathPrefix = NULL);
}


namespace TrafficGeneratorClient{
void startClient(appByte *serverIp, appInt serverPort, appByte *fpath);
}

namespace TrafficGeneratorClient2{
void startClient(appByte *serverIp, appInt serverPort, appInt numThread, appInt numPackets);
}
#endif /* TEST_TRAFFICGENERATOR_H_ */
