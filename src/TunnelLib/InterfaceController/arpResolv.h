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
