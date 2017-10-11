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
 * arpResolv.h
 *
 *  Created on: 28-Feb-2017
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_INTERFACECONTROLLER_ARPRESOLV_H_
#define SRC_TUNNELLIB_INTERFACECONTROLLER_ARPRESOLV_H_

#include <common.h>

#ifdef __cplusplus
extern "C" {
#endif


int arpmain(appInt32 srcIp, appInt32 remoteIp, appSInt ifid);

#ifdef __cplusplus
}
#endif

#endif /* SRC_TUNNELLIB_INTERFACECONTROLLER_ARPRESOLV_H_ */
