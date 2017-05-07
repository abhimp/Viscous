/*
 * TcpMultiplexingTrafficGenerator.h
 *
 *  Created on: 14-Apr-2017
 *      Author: abhijit
 */

#ifndef TEST_TCPMULTIPLEXINGTRAFFICGENERATOR_H_
#define TEST_TCPMULTIPLEXINGTRAFFICGENERATOR_H_
#include <common.h>

namespace TcpMultiplexing {
namespace Sender {
void startClient(appByte *serverIp, appInt serverPort, appInt numThread, appInt numConn);
}
} /* namespace TcpMultiplexing */

#endif /* TEST_TCPMULTIPLEXINGTRAFFICGENERATOR_H_ */
