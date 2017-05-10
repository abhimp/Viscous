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
