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
 * log.hh
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
