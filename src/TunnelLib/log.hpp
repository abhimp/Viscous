/*
 * log.hpp
 *
 *  Created on: 17-Jan-2017
 *      Author: sourav
 */

#ifndef SRC_TUNNELLIB_LOG_HPP_
#define SRC_TUNNELLIB_LOG_HPP_
#include <string>
//#define DEBUG true
#define LOGFILE "mpudp.log"
void LOG(std::string fmt,...);
#define LOG_PRINT(...) LOG(__FILE__, __LINE__, __VA_ARGS__ )

#endif /* SRC_TUNNELLIB_LOG_HPP_ */
