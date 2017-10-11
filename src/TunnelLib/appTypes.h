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
 * appTypes.h
 *
 *  Created on: 23-Feb-2017
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_APPTYPES_H_
#define SRC_TUNNELLIB_APPTYPES_H_
namespace APP_TYPE{

typedef enum{
    APP_GET_STREAM_FLOW,
    APP_GET_STREAM_FLOW_AND_FINGER_PRINT
}APP_GET_OPTION;

}


#endif /* SRC_TUNNELLIB_APPTYPES_H_ */
