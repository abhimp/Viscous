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
 * TcpMultiplexingTrafficGenerator.h
 *
 *  Created on: 14-Apr-2017
 *      Author: abhijit
 */

#ifndef EVALUATION_TCPMULTIPLEXINGTRAFFICGENERATOR_H_
#define EVALUATION_TCPMULTIPLEXINGTRAFFICGENERATOR_H_
#include <common.h>

namespace TcpMultiplexing {
namespace Sender {
void startClient(appByte *serverIp, appInt serverPort, appInt numThread, appInt numConn);
}
} /* namespace TcpMultiplexing */

#endif /* EVALUATION_TCPMULTIPLEXINGTRAFFICGENERATOR_H_ */
