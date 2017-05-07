/*
 * TcpTrafficGenerator.h
 *
 *  Created on: 17-Mar-2017
 *      Author: abhijit
 */

#ifndef TEST_TCPTRAFFICGENERATOR_H_
#define TEST_TCPTRAFFICGENERATOR_H_

#include <common.h>

namespace TcpTrafficGeneratorReciever{
void startServer(appInt localPort, appByte *fylePathPrefix);
}

namespace TcpTrafficGeneratorSender{
void startClient(appByte *serverIp, appInt serverPort, appByte *fpath);
}

namespace TcpTrafficGeneratorSender2{
    void startClient(appByte *serverIp, appInt serverPort, appInt numThread, appInt numConn);
}

#endif /* TEST_TCPTRAFFICGENERATOR_H_ */
